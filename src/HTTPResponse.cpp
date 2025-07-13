#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"
#include <cstddef>
#include <stdexcept>
#include <map>
#include "Webserv.hpp"
#include "Location.hpp"

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
	HTTPRequest::e_method method = request.get_method();

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
	switch (method)
	{
		case HTTPRequest::GET:
			handle_get(request);
			break;
		case HTTPRequest::POST:
			// Temporary stub.
			_status_code = 405;
			this->build_error_response();
			break;
		case HTTPRequest::DELETE:
			// Temporary stub.
			_status_code = 405;
			this->build_error_response();
			break;
		default:
			// This should never occur.
			_status_code = 500;
			this->build_error_response();
	}
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

std::string HTTPResponse::resolve_secure_path(const std::string& request_path) const
{
	// Step 1: Normalize input.
	std::string path = request_path.empty() ? "/" : request_path;
	std::string::size_type pos = path.find_first_of("?#");

	if (pos != std::string::npos)
	{
		path = path.substr(0, pos);
	}
	if (!path.empty() && path[0] == '/')
	{
		path = path.substr(1);
	}

	std::string full_path = _root + "/" + path;
	// Step 2: Canonicalize full_path.
	char resolved[PATH_MAX];
	if (!realpath(full_path.c_str(), resolved))
	{
		throw HTTPError(404, "resolve_secure_path: unable to resolve file path: " + full_path);
	}
	std::string resolved_path(resolved);

	// Step 3: Canonicalize _root
	char canonical_root[PATH_MAX];
	if (!realpath(_root.c_str(), canonical_root))
	{
		throw HTTPError(500, "resolve_secure_path: failed to resolve server root path: " + _root);
	}
	std::string resolved_root(canonical_root);

	// Step 4: Ensure resolved_path is within resolved_root (security check)
	if (resolved_path.compare(0, resolved_root.size(), resolved_root) != 0 ||
	    (resolved_path.size() > resolved_root.size() && resolved_path[resolved_root.size()] != '/'))
	{
		throw HTTPError(403, "resolve_secure_path: directory traversal attempt detected.");
	}
	return resolved_path;
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

void HTTPResponse::handle_get(const HTTPRequest& request)
{
	// Location pointer.
	const Location *lp;
	std::string request_relative_path;
	std::string resolved_path;

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
	// Checking if "GET" method is allowed.
	if (lp != NULL
		&& lp->getMethods().find("GET") == lp->getMethods().end())
	{
		_status_code = 405;
		build_error_response();
		return;
	}
	// Getting request relative path.
	if (lp != NULL)
	{
		request_relative_path = request.get_request_path_decoded_strip_location_path(
				lp->getPath());
	}
	else
	{
		request_relative_path = request.get_request_path_decoded();
		// Request path initial '/'.
		request_relative_path.erase(0, 1);
	}
	try
	{
		if (lp != NULL)
		{
			resolved_path = this->resolve_path(lp->getPath(),
					request_relative_path);
		}
		else
		{
			resolved_path = this->resolve_path(_server_cfg->getRoot(),
					request_relative_path);
		}
	}
	catch (const directory_traversal_detected &e)
	{
		_status_code = 406;	// I guess 406 is good in this case?
		build_error_response();
		return;
	}
	// TODO.
	_headers["Connection"] = "close";
	_response_ready = true;



	/*
	const std::vector<Location> loc = server_config.getLocations();	// Unused for now, but should be checked later.

        try
	{
		std::string requested_path = request.get_request_target();
		std::string full_path = resolve_secure_path(requested_path);

		// If it's a directory, try to append "/index.html"
		if (isDirectory(full_path)) {
			if (!full_path.empty() && full_path[full_path.length() - 1] != '/')
				full_path += '/';
			full_path += "index.html";
		}
		// File must exist
		if (!pathExists(full_path)) {
			_status_code = 404; // Not found.
			_response_msg = generateErrorPage(_status_code);
			return;
		}
		// Must not be a directory (even after index.html fallback)
		if (isDirectory(full_path)) {
			_status_code = 403;
			_response_msg = generateErrorPage(_status_code);
			return;
		}
		// Must have read permissions
		if (!has_permission(full_path, HTTPRequest::GET)) {
			_status_code = 403; // Forbidden.
			_response_msg = generateErrorPage(_status_code);
			return;
		}
		// Read file contents
		std::ifstream file(full_path.c_str());
		if (!file)
		{
			_status_code = 500; // Internal Server Error
			_response_msg = generateErrorPage(_status_code);
			return;
		}
		_mime = get_mime_type(full_path);
		std::ostringstream content;
		content << file.rdbuf();
		_response_body = content.str();
		_status_code = 200;
	}
	catch (const HTTPResponse::HTTPError& err)
	{
		print_err("Routing error: ", err.what(), " (HTTP " + to_string(err.code) + ")");
		_status_code = err.code;
		_response_msg = generateErrorPage(_status_code);
		_response_ready = true; // Set response ready even on error.
	}
	catch (const std::exception& e)
	{
		print_err("handleGET error: ", e.what(), "");
		_status_code = 500;
		_response_msg = generateErrorPage(_status_code);
		_response_ready = true; // Set response ready even on error
	}
	*/
}

void HTTPResponse::append_required_headers()
{
	_headers["Server"] = "hlyshchu_asagymba";
	_headers["Content_Length"] = to_string(_response_body.length());
}

std::string HTTPResponse::resolve_path(const std::string &root,
		const std::string &request_relative_path) const
{
	// Checking for possible directory traversal in `request_relative_path`.
	size_t depth = 0;
	bool in_directory = false;

	if (request_relative_path.length() != 0
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
