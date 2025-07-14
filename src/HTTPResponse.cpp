#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"
#include <cstddef>
#include <stdexcept>
#include <map>
#include "Webserv.hpp"
#include "Location.hpp"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

HTTPResponse::HTTPResponse()
	: _server_cfg(NULL),
	  _status_code(100),		// Temporary code.
	  _response_ready(false)
{
}

HTTPResponse::HTTPResponse(int status_code)
	: _server_cfg(NULL),
	  _status_code(status_code),
	  _response_ready(false)
{
}

HTTPResponse::HTTPResponse(const HTTPResponse &other)
	: _server_cfg(other._server_cfg),
	  _status_code(other._status_code),
	  _headers(other._headers),
	  _response_body(other._response_body),
	  _response_ready(other._response_ready)
{
}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse &other)
{
	if (this == &other)
	{
		return *this;
	}
	_server_cfg = other._server_cfg;
	_status_code = other._status_code;
	_headers = other._headers;
	_response_body = other._response_body;
	_response_ready = other._response_ready;
	return *this;
}

HTTPResponse::~HTTPResponse()
{
}

HTTPResponse::directory_traversal_detected::directory_traversal_detected(
		const char * msg)
	: m_msg(msg)
{
}

HTTPResponse::directory_traversal_detected::directory_traversal_detected(
		const std::string &msg)
	: m_msg(msg.c_str())
{
}

const char * HTTPResponse::directory_traversal_detected::what() const throw()
{
	return m_msg;
}

void HTTPResponse::set_server_cfg(ServerConfig *server_cfg)
{
	if (server_cfg == NULL)
	{
		throw std::invalid_argument(std::string("HTTPResponse::set_server_cfg(): ")
				+ "server_cfg can't be NULL.");
	}
	_server_cfg = server_cfg;
}

void HTTPResponse::build_error_response()
{
	if (_server_cfg == NULL)
	{
		throw std::runtime_error(std::string("HTTPResponse::build_error_response(): ")
				+ "server_cfg can't be NULL.");
	}
	else if (_response_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::build_error_response(): ")
				+ "Response message is already prepared.");
	}

	std::map<int, std::string>::const_iterator it = _server_cfg->getErrorPages().find(_status_code);

	_headers["Connection"] = "close";
	if (it != _server_cfg->getErrorPages().end())
	{
		try
		{
			_response_body = read_file(it->second);
			// If MIME is not HTML,
			// then something is definitely wrong.
			_headers["Content-Type"] = get_mime_type(it->second);
			_response_ready = true;
			return;
		}
		catch (const std::ios_base::failure &e)
		{
			print_warning("Couldn't read error page: ", e.what(), "");
		}
	}
	// Either the error page for this error code doesn't exist,
	// or file reading failed.
	_response_body = generateErrorBody(_status_code);
	_headers["Content-Type"] = "text/html";
	_response_ready = true;
}

void HTTPResponse::handle_response_routine(const HTTPRequest &request)
{
	// Pointer to Location corresponding to request path in `request`.
	const Location *lp;
	// Request handler method.
	void (HTTPResponse::*handler)(const HTTPRequest &request,
			const Location *lp,
			std::string &request_dir_relative_to_root,
			std::string &request_location_path,
			std::string &request_dir_root,
			std::string &resolved_path);
	// Request dirs, paths, etc.
	std::string request_dir_relative_to_root;
	std::string request_location_path;
	std::string request_dir_root;
	std::string resolved_path;

	// Checking for usage errors.
	if (_server_cfg == NULL)
	{
		throw std::runtime_error(std::string("HTTPResponse::handle_response_routine(): ")
				+ "server_cfg can't be NULL.");
	}
	else if (_response_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::handle_response_routine(): ")
				+ "Response message is already prepared.");
	}
	// Getting Location pointer.
	try
	{
		const Location &loc = _server_cfg->determineLocation(
				request.get_request_path_decoded());

		lp = &loc;
	}
	catch (const std::out_of_range &e)
	{
		lp = NULL;
	}
	// Setting handler.
	switch (request.get_method())
	{
		case HTTPRequest::GET:
			if (lp != NULL
				&& lp->getMethods().find("GET")
					== lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			handler = &HTTPResponse::handle_get;
			break;
		case HTTPRequest::POST:
			if (lp == NULL
				|| lp->getMethods().find("POST")
					== lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			// Temporary stub.
			_status_code = 405;
			this->build_error_response();
			return;
			break;
		case HTTPRequest::DELETE:
			if (lp == NULL
				|| lp->getMethods().find("DELETE")
					== lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			// Temporary stub.
			_status_code = 405;
			this->build_error_response();
			return;
			break;
		default:
			// This should never occur.
			_status_code = 500;
			this->build_error_response();
			return;
	}
	// Resolving request dirs, paths, etc.
	// Ideally, this should be in it's own method...
	if (lp != NULL)
	{
		request_dir_relative_to_root = request.get_request_path_decoded_strip_location_path(
				lp->getPath());
		request_location_path = lp->getPath();
		if (!(lp->getRootLocation().empty()))
		{
			request_dir_root = lp->getRootLocation();
		}
		else if (!(lp->getAlias().empty()))
		{
			request_dir_root = lp->getAlias();
		}
		else
		{
			request_dir_root = _server_cfg->getRoot();
		}
	}
	else
	{
		request_dir_relative_to_root = request.get_request_path_decoded();
		// Request path initial '/'.
		request_dir_relative_to_root.erase(0, 1);
		request_location_path = '/';
		request_dir_root = _server_cfg->getRoot();
	}
	if (request_dir_root.length() <= 0
		|| request_dir_root.at(request_dir_root.length() - 1) != '/')
	{
			request_dir_root.push_back('/');
	}
	try
	{
		resolved_path = this->resolve_path(request_dir_root,
				request_dir_relative_to_root);
	}
	catch (const directory_traversal_detected &e)
	{
		_status_code = 406;	// I guess 406 is good in this case?
		build_error_response();
		return;
	}
	(this->*handler)(request, lp,
			request_dir_relative_to_root, request_location_path,
			request_dir_root, resolved_path);
}

bool HTTPResponse::is_response_ready() const
{
	return _response_ready;
}

std::string HTTPResponse::get_response_msg()
{
	std::ostringstream response;

	if (!_response_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::get_response_msg(): ")
				+ "Response message isn't ready yet.");
	}
	// Start line.
	response << "HTTP/1.1 " << _status_code << " " << getReasonPhrase(_status_code) << "\r\n";
	// Taking care of header fields that must always be present,
	// but that may be not set by `build_error_response()`
	// or `handle_response_routine()`.
	this->append_required_headers();
	// Headers.
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << "\r\n";
	}
	response << "\r\n";		// End of headers.
	response << _response_body;	// Body content.
	return response.str();
}

bool HTTPResponse::should_close_connection() const
{
	if (!_response_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::should_close_connection(): ")
				+ "Response message isn't ready yet.");
	}
	else if (_headers.find("Connection") != _headers.end()
			&& _headers.at("Connection").compare("close") == 0)
	{
		return true;
	}
	return false;
}

/*
bool HTTPResponse::has_permission(const std::string& path, HTTPRequest::e_method method) const
{
	int mode = 0;

	switch (method)
	{
		case HTTPRequest::GET:
			mode = R_OK; // Read permission.
			break;
		case HTTPRequest::POST:
			mode = W_OK; // Write permission (for appending/uploading).
			break;
		case HTTPRequest::DELETE:
			mode = W_OK; // Need write access to the directory (delete op).
			break;
		default:
			return false;
	}
	return (access(path.c_str(), mode) == 0);
}
 */

std::string HTTPResponse::get_mime_type(const std::string &path)
{
	static std::map<std::string, std::string> mime_map;

	if (mime_map.empty())
	{
		mime_map.insert(std::make_pair(".html", "text/html"));
		mime_map.insert(std::make_pair(".htm", "text/html"));
		mime_map.insert(std::make_pair(".css", "text/css"));
		mime_map.insert(std::make_pair(".js", "application/javascript"));
		mime_map.insert(std::make_pair(".png", "image/png"));
		mime_map.insert(std::make_pair(".jpg", "image/jpeg"));
		mime_map.insert(std::make_pair(".jpeg", "image/jpeg"));
		mime_map.insert(std::make_pair(".gif", "image/gif"));
		mime_map.insert(std::make_pair(".svg", "image/svg+xml"));
		mime_map.insert(std::make_pair(".json", "application/json"));
		mime_map.insert(std::make_pair(".pdf", "application/pdf"));
		mime_map.insert(std::make_pair(".txt", "text/plain"));
		mime_map.insert(std::make_pair(".xml", "application/xml"));
	}
	std::string::size_type dot = path.find_last_of('.');
	if (dot == std::string::npos)
	{
		return "application/octet-stream";
	}
	std::string ext = path.substr(dot);
	for (size_t i = 0; i < ext.size(); ++i)
	{
		ext[i] = std::tolower(static_cast<unsigned char> (ext[i]));
	}
	std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
	if (it != mime_map.end())
	{
		return it->second;
	}
	return "application/octet-stream";
}

void HTTPResponse::append_required_headers()
{
	_headers["Server"] = "hlyshchu_asagymba";
	_headers["Content-Length"] = to_string(_response_body.length());
}

void HTTPResponse::handle_get(const HTTPRequest &request, const Location *lp,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &request_dir_root,
		std::string &resolved_path)
{
	if (isDirectory(resolved_path))
	{
		if (request_dir_relative_to_root.length() > 0
			&& request_dir_relative_to_root.at(
				request_dir_relative_to_root.length() - 1) != '/')
		{
			generate_301(request_location_path
				+ request_dir_relative_to_root + '/');
			set_connection_header(request);
			_response_ready = true;
			print_log("Sent the 301: ", _response_body, "");
			return;
		}
		else if (find_first_available_index(lp,
					request_dir_root,
					request_dir_relative_to_root) == 0)
		{
			// find_first_available_index() found
			// an available index and appended it's location
			// relative to `request_dir_root`
			// to `request_dir_relative_to_root`.
			// Update `resolved_path` in this case.
			resolved_path = request_dir_root
				+ request_dir_relative_to_root;
		}
		// No available index was found.
		else if (lp->getAutoindex() == true)
		{
			try
			{
				generate_auto_index(resolved_path);
			}
			catch (const std::ios_base::failure &e)
			{
				_status_code = 500;
				build_error_response();
				return;
			}
			set_connection_header(request);
			_response_ready = true;
			print_log("Sent the autoindex at: ", resolved_path, "");
			return;
		}
		// No available index was found AND autoindex is off.
		else
		{
			_status_code = 403;
			build_error_response();
			return;
		}
	}
	if (!pathExists(resolved_path))
	{
		_status_code = 404;
		build_error_response();
		return;
	}
	else if (access(resolved_path.c_str(), R_OK) == -1)
	{
		_status_code = 403;
		build_error_response();
		return;
	}
	// TODO (CGI part will start here).
	try
	{
		_response_body = read_file(resolved_path);
	}
	catch (const std::ios_base::failure &e)
	{
		_status_code = 500;
		build_error_response();
		return;
	}
	_status_code = 200;
	_headers["Content-Type"] = get_mime_type(resolved_path);
	set_connection_header(request);
	print_log("Sending ", resolved_path, " to the server");
	_response_ready = true;
}

std::string HTTPResponse::resolve_path(const std::string &root,
		const std::string &request_relative_path) const
{
	// Checking for possible directory traversal in `request_relative_path`.
	size_t depth = 0;
	bool in_directory = false;

	if (request_relative_path.length() > 0
		&& request_relative_path.at(0) == '/')
	{
		throw std::invalid_argument(std::string("HTTPResponse::resolve_path(): ")
				+ "Provided request path is not relative: "
				+ request_relative_path);
	}
	for (size_t i = 0; i < request_relative_path.length(); i++)
	{
		if (request_relative_path.at(i) == '.'
			&& !in_directory
			&& (i + 1 < request_relative_path.length()
				&& request_relative_path.at(i + 1) == '.'))
		{
			// More than 3 levels of indentation is bad.
			// Still, let's proceed as is...
			if (depth-- == 0)
			{
				throw directory_traversal_detected(std::string(
							"HTTPResponse::resolve_path(): ")
						+ "Detected directory traversal.");
			}
			i++;	// Skip the second dot.
		}
		else if (request_relative_path.at(i) == '/')
		{
			// Not checking for double slashes.
			// Double slashes in the request path
			// are already checked for during HTTPRequest parsing.
			if (in_directory)
			{
				depth++;
			}
			in_directory = false;
		}
		else
		{
			in_directory = true;
		}
	}
	return root + request_relative_path;
}

void HTTPResponse::generate_301(const std::string &redir_path)
{
	_status_code = 301;
	_headers["Location"] = redir_path;
	_response_body = "Moved_permanently to ";
	_response_body += redir_path;
	_response_body += '\n';
}

int HTTPResponse::find_first_available_index(const Location *lp,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root) const
{
	const std::vector<std::string> *indexes = NULL;
	std::string index_path;

	if (lp != NULL)
	{
		indexes = &(lp->getIndexLocation());
		if (indexes->size() == 0)
		{
			indexes = &(_server_cfg->getIndex());
		}
	}
	else
	{
		indexes = &(_server_cfg->getIndex());
	}
	for (size_t i = 0; i < indexes->size(); i++)
	{
		if ((indexes->at(i)).compare(0,
				request_dir_relative_to_root.length(),
				request_dir_relative_to_root) == 0)
		{
			index_path = request_dir_root + indexes->at(i);
			if (isRegFile(index_path)
				&& access(index_path.c_str(), R_OK) == 0)
			{
				// Instead of writing an append logic,
				// it's easier just to copy the whole index path.
				request_dir_relative_to_root = indexes->at(i);
				return 0;
			}
		}
	}
	return -1;
}

void HTTPResponse::generate_auto_index(const std::string &path)
{
	DIR *dir;
	struct dirent *dir_ent;
	// If there are no available entries (file or directories)
	// in `path`, state that in the response clearly.
	bool have_at_least_one_entry = false;

	_status_code = 200;
	_headers["Content-Type"] = "text/html";
	dir = opendir(path.c_str());
	if (dir == NULL)
	{
		throw std::ios_base::failure(std::string("HTTPResponse::generate_auto_index: ")
				+ "Couldn't open the directory at: " + path);
	}
	errno = 0;
	_response_body = "<html>\n";
	_response_body += "<head>\n";
	_response_body += "<title>Index</title>\n";
	_response_body += "</head>\n";
	_response_body += "<body>\n";
	while ((dir_ent = readdir(dir)) != NULL)
	{
		// Let's skip those, ok?
		if (strcmp(dir_ent->d_name, ".") == 0
			|| strcmp(dir_ent->d_name, "..") == 0)
		{
			continue;
		}
		_response_body += "<a href=\"";
		_response_body += + dir_ent->d_name;
		_response_body += "\">";
		_response_body += dir_ent->d_name;
		_response_body += "</a><br />\n";
		have_at_least_one_entry = true;
	}
	if (errno != 0)
	{
		throw std::ios_base::failure(std::string("HTTPResponse::generate_auto_index: ")
				+ "Couldn't read the directory at: " + path);
	}
	else if (closedir(dir) == -1)
	{
		throw std::ios_base::failure(std::string("HTTPResponse::generate_auto_index: ")
				+ "Couldn't close the directory at: " + path);
	}
	if (!have_at_least_one_entry)
	{
		_response_body += "<b>No entries</b> in this directory.<br />\n";
	}
	_response_body += "</body>\n";
	_response_body += "</html>\n";
}

void HTTPResponse::set_connection_header(const HTTPRequest &request)
{
	try
	{
		_headers["Connection"] = request.get_header_value(
				"Connection");
	}
	catch (const std::range_error &e)
	{
		_headers["Connection"] = "keep-alive";
	}
}
