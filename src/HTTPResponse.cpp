#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"
#include <sys/types.h>
#include <cstddef>
#include <stdexcept>
#include <map>
#include <csignal>
#include "Webserv.hpp"
#include "Location.hpp"
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>

// To set up envp() for CGI.
extern char **environ;

HTTPResponse::HTTPResponse()
	: _server_cfg(NULL),
	  _status_code(100),		// Temporary code.
	  _payload_ready(false),
	  _lp(NULL),
	  _cgi_pid(-1),
	  _cgi_launch_time(0)
{
	_cgi_pipe[0] = -1;
	_cgi_pipe[1] = -1;
}

HTTPResponse::HTTPResponse(int status_code)
	: _server_cfg(NULL),
	  _status_code(status_code),
	  _payload_ready(false),
	  _lp(NULL),
	  _cgi_pid(-1),
	  _cgi_launch_time(0)
{
	_cgi_pipe[0] = -1;
	_cgi_pipe[1] = -1;
}

HTTPResponse::HTTPResponse(const HTTPResponse &other)
	: _server_cfg(other._server_cfg),
	  _status_code(other._status_code),
	  _headers(other._headers),
	  _response_body(other._response_body),
	  _payload(other._payload),
	  _payload_ready(other._payload_ready),
	  _lp(other._lp),
	  _cgi_pid(-1),
	  _cgi_launch_time(0)
{
	_cgi_pipe[0] = -1;
	_cgi_pipe[1] = -1;
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
	_payload = other._payload;
	_payload_ready = other._payload_ready;
	_lp = other._lp;
	if (_cgi_pid != -1)
	{
		kill(_cgi_pid, SIGTERM);
	}
	if (_cgi_pipe[0] != -1)
	{
		close(_cgi_pipe[0]);
	}
	if (_cgi_pipe[1] != -1)
	{
		close(_cgi_pipe[1]);
	}
	_cgi_launch_time = 0;
	return *this;
}

HTTPResponse::~HTTPResponse()
{
}

HTTPResponse::directory_traversal_detected::directory_traversal_detected(
		const char * msg)
	: _MSG(msg)
{
}

HTTPResponse::directory_traversal_detected::directory_traversal_detected(
		const std::string &msg)
	: _MSG(msg)
{
}

HTTPResponse::directory_traversal_detected::~directory_traversal_detected() throw()
{
}

const char * HTTPResponse::directory_traversal_detected::what() const throw()
{
	return _MSG.c_str();
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
	// `it` is a helper to construct `error_page_path`.
	std::map<int, std::string>::const_iterator it;
	std::string error_page_path;

	if (_server_cfg == NULL)
	{
		throw std::runtime_error(std::string("HTTPResponse::build_error_response(): ")
				+ "server_cfg can't be NULL.");
	}
	else if (_payload_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::build_error_response(): ")
				+ "Response message is already prepared.");
	}
	// Resolving `error_page_path`.
	if (_lp != NULL)
	{
		it = _lp->getErrorPages().find(_status_code);
		if (it != _lp->getErrorPages().end())
		{
			// This is so stupid, this is actually so stupid.
			if (_lp->getRootLocation().length() != 0)
			{
				error_page_path = _lp->getRootLocation();
			}
			else
			{
				error_page_path = _lp->getAlias();
			}
			if (error_page_path.at(error_page_path.length() - 1)
				!= '/')
			{
				error_page_path.push_back('/');
			}
			error_page_path += it->second;
		}
	}
	// Still resolving `error_page_path.
	if (_lp == NULL || it == _lp->getErrorPages().end())
	{
		it = _server_cfg->getErrorPages().find(_status_code);
		if (it != _server_cfg->getErrorPages().end())
		{
			error_page_path = _server_cfg->getRoot();
			if (error_page_path.at(error_page_path.length() - 1)
				!= '/')
			{
				error_page_path.push_back('/');
			}
			error_page_path += it->second;
		}
	}
	// "Connection" header.
	if (_headers.find("Connection") == _headers.end())
	{
		_headers["Connection"] = "close";
	}
	// Sending the error page itself.
	if (error_page_path.length() > 0)
	{
		try
		{
			_response_body = read_file(error_page_path);
			// If MIME is not HTML,
			// then something is definitely wrong.
			_headers["Content-Type"] = get_mime_type(error_page_path);
			this->prep_payload();
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
	this->prep_payload();
}

void HTTPResponse::handle_response_routine(const HTTPRequest &request)
{
	// Request handler method.
	void (HTTPResponse::*handler)(const HTTPRequest &request,
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
	else if (_payload_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::handle_response_routine(): ")
				+ "Response message is already prepared.");
	}
	// Getting Location pointer.
	try
	{
		const Location &loc = _server_cfg->determineLocation(
				request.get_request_path_decoded());

		_lp = &loc;
	}
	catch (const std::out_of_range &e)
	{
		_lp = NULL;
	}
	// Setting handler.
	switch (request.get_method())
	{
		case HTTPRequest::GET:
			if (_lp != NULL
				&& _lp->getMethods().find("GET")
					== _lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			handler = &HTTPResponse::handle_get;
			break;
		case HTTPRequest::POST:
			if (_lp == NULL
				|| _lp->getMethods().find("POST")
					== _lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			handler = &HTTPResponse::handle_post;
			break;
		case HTTPRequest::DELETE:
			if (_lp == NULL
				|| _lp->getMethods().find("DELETE")
					== _lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			handler = &HTTPResponse::handle_delete;
			break;
		case HTTPRequest::PUT:
			if (_lp == NULL
				|| _lp->getMethods().find("PUT")
					== _lp->getMethods().end())
			{
				_status_code = 405;
				build_error_response();
				return;
			}
			handler = &HTTPResponse::handle_put;
			break;
		default:
			// This should never occur.
			_status_code = 500;
			this->build_error_response();
			return;
	}
	// Resolving request dirs, paths, etc.
	// Ideally, this should be in it's own method...
	if (_lp != NULL)
	{
		request_dir_relative_to_root = request.get_request_path_decoded_strip_location_path(
				_lp->getPath());
		request_location_path = _lp->getPath();
		// In Location, at least one of `_root` or `_alias`
		// must be defined.
		if (!(_lp->getRootLocation().empty()))
		{
			request_dir_root = _lp->getRootLocation();
		}
		else
		{
			request_dir_root = _lp->getAlias();
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
		print_err("Detected directory traversal attempt: ", e.what(), "");
		// I guess 403 or 406 is good in this case?
		_status_code = 403;
		build_error_response();
		return;
	}
	(this->*handler)(request,
			request_dir_root, request_dir_relative_to_root,
			request_location_path, resolved_path);
}

bool HTTPResponse::is_response_ready() const
{
	return _payload_ready;
}

const std::string &HTTPResponse::get_response_msg() const
{
	if (!_payload_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::get_response_msg(): ")
				+ "Response payload isn't ready yet.");
	}
	return _payload;
}

bool HTTPResponse::should_close_connection() const
{
	if (!_payload_ready)
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

void HTTPResponse::prep_payload()
{
	std::ostringstream payload;

	if (_payload_ready)
	{
		throw std::runtime_error(std::string("HTTPResponse::prep_payload(): ")
				+ "Response payload is already prepared.");
	}
	// Start line.
	payload << "HTTP/1.1 " << _status_code << " " << getReasonPhrase(_status_code) << "\r\n";
	// Taking care of header fields that must always be present,
	// but that may be not set by `build_error_response()`
	// or `handle_response_routine()`.
	this->append_required_headers();
	// Headers.
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
	{
		payload << it->first << ": " << it->second << "\r\n";
	}
	payload << "\r\n";		// End of headers.
	payload << _response_body;	// Body content.
	_payload = payload.str();
	_payload_ready = true;
}

void HTTPResponse::append_required_headers()
{
	_headers["Server"] = SERVER_NAME;
	_headers["Content-Length"] = to_string(_response_body.length());
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
				throw directory_traversal_detected(std::string("HTTPResponse::resolve_path(): ")
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

void HTTPResponse::handle_get(const HTTPRequest &request,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &resolved_path)
{
	int cgi_status;

	if (isDirectory(resolved_path))
	{
		if (request_dir_relative_to_root.length() > 0
			&& request_dir_relative_to_root.at(
				request_dir_relative_to_root.length() - 1) != '/')
		{
			generate_301(request_location_path
				+ request_dir_relative_to_root + '/');
			set_connection_header(request);
			prep_payload();
			print_log("Sent the 301: ", _response_body, "");
			return;
		}
		else if (find_first_available_index(
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
		else if (_lp->getAutoindex() == true)
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
			prep_payload();
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
	// At this point, `resolved_path` must be a file
	// (at least not a directory).
	else if (access(resolved_path.c_str(), R_OK) == -1)
	{
		_status_code = 403;
		print_log("HTTPResponse::handle_get(): Can't read file at: ",
			resolved_path, "");
		build_error_response();
		return;
	}
	if (_lp != NULL
		&& std::find(_lp->getCgiExtension().begin(),
			_lp->getCgiExtension().end(),
			get_file_ext(resolved_path))
			!= _lp->getCgiExtension().end())
	{
		cgi_status = handle_cgi(request,
				request_dir_root, request_dir_relative_to_root,
				request_location_path, resolved_path);
		if (cgi_status != 0)
		{
			_status_code = cgi_status;
			build_error_response();
		}
		return;
	}
	try
	{
		_response_body = read_file(resolved_path);
	}
	catch (const std::ios_base::failure &e)
	{
		_status_code = 500;
		print_warning("HTTPResponse::handle_get(): I/O error: ",
			e.what(), "");
		build_error_response();
		return;
	}
	_status_code = 200;
	_headers["Content-Type"] = get_mime_type(resolved_path);
	set_connection_header(request);
	prep_payload();
	print_log("Sending ", resolved_path, " to the server");
}

void HTTPResponse::handle_post(const HTTPRequest &request,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &resolved_path)
{
	int cgi_status;

	if (isDirectory(resolved_path))
	{
		if (request_dir_relative_to_root.length() > 0
			&& request_dir_relative_to_root.at(
				request_dir_relative_to_root.length() - 1) != '/')
		{
			generate_301(request_location_path
				+ request_dir_relative_to_root + '/');
			set_connection_header(request);
			prep_payload();
			print_log("Sent the 301: ", _response_body, "");
			return;
		}
		else if (find_first_available_index(
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
		else
		{
			generate_204('/' + request_dir_relative_to_root);
			set_connection_header(request);
			prep_payload();
			print_log("Sending the 204:\n", _payload, "\n");
			return;
		}
	}
	if (!pathExists(resolved_path))
	{
		// Should it be 404 in this case?
		generate_204('/' + request_dir_relative_to_root);
		set_connection_header(request);
		prep_payload();
		print_log("Sending the 204:\n", _payload, "\n");
		return;
	}
	// At this point, `resolved_path` must be a file
	// (at least not a directory).
	else if (access(resolved_path.c_str(), R_OK) == -1)
	{
		_status_code = 403;
		print_log("HTTPResponse::handle_post(): Can't read file at: ",
			resolved_path, "");
		build_error_response();
		return;
	}
	if (_lp != NULL
		&& std::find(_lp->getCgiExtension().begin(),
			_lp->getCgiExtension().end(),
			get_file_ext(resolved_path))
			!= _lp->getCgiExtension().end())
	{
		cgi_status = handle_cgi(request,
				request_dir_root, request_dir_relative_to_root,
				request_location_path, resolved_path);
		if (cgi_status != 0)
		{
			_status_code = cgi_status;
			build_error_response();
		}
		return;
	}
	else if (access(resolved_path.c_str(), W_OK) == -1)
	{
		_status_code = 403;
		print_log("HTTPResponse::handle_post(): Can't write to file at: ",
			resolved_path, "");
		build_error_response();
		return;
	}
	try
	{
		append_file(resolved_path, request.get_body());
	}
	catch (const std::ios_base::failure &e)
	{
		_status_code = 500;
		print_warning("HTTPResponse::handle_post(): I/O error: ",
			e.what(), "");
		build_error_response();
		return;
	}
	generate_204('/' + request_dir_relative_to_root);
	set_connection_header(request);
	prep_payload();
	print_log("Sending the 204:\n", _payload, "\n");
}

// RFC:
// ... If the target resource has one or more current representations,
// they might or might not be destroyed by the origin server,
// and the associated storage might or might not be reclaimed,
// depending entirely on the nature of the resource and
// its implementation by the origin server
// (which are beyond the scope of this specification). ...
//
// We therefore can safely drop the implementation of directory deletion.
void HTTPResponse::handle_delete(const HTTPRequest &request,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &resolved_path)
{
	(void) request_dir_root;
	int remove_status;

	if (isDirectory(resolved_path))
	{
		if (request_dir_relative_to_root.length() > 0
			&& request_dir_relative_to_root.at(
				request_dir_relative_to_root.length() - 1) != '/')
		{
			generate_301(request_location_path
				+ request_dir_relative_to_root + '/');
			set_connection_header(request);
			prep_payload();
			print_log("Sent the 301: ", _response_body, "");
			return;
		}
		// Let's rather NOT delete any indexes in this case
		// and just return 403 (we don't have to handle directory
		// removal, it's not stated anywhere in RFC).
		/*
		else if (find_first_available_index(
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
		 */
		else
		{
			_status_code = 403;
			print_log("Got DELETE request to delete: ",
				resolved_path, " - can't delete directory");
			build_error_response();
			return;
		}
	}
	if (!pathExists(resolved_path))
	{
		// Should it be 204 in this case?
		_status_code = 404;
		print_log("Got DELETE request to delete: ",
			resolved_path, " - path doesn't exist");
		build_error_response();
		return;
	}
	errno = 0;
	if ((remove_status = std::remove(resolved_path.c_str())) != 0)
	{
		if (remove_status == EACCES || errno == EACCES)
		{
			_status_code = 403;
			print_log("Got DELETE request to delete: ",
				resolved_path, " - permission denied");
			build_error_response();
			return;
		}
		else
		{
			_status_code = 500;
			print_log("Got DELETE request to delete: ",
				resolved_path, " - error other than EACCES");
			build_error_response();
			return;
		}
	}
	generate_204('/' + request_dir_relative_to_root);
	set_connection_header(request);
	prep_payload();
	print_log("Sending the 204:\n", _payload, "\n");
}

void HTTPResponse::handle_put(const HTTPRequest &request,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &resolved_path)
{
	std::ofstream file;

	// Handling "upload_path" config directive.
	if (!_lp->getUploadPath().empty())
	{
		// This is kinda weird to set
		// `request_dir_root` to `_lp->_upload_path`,
		// but everything works, so...
		request_dir_root = _lp->getUploadPath();
		if (request_dir_root.at(request_dir_root.length() - 1) != '/')
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
			print_err("Detected directory traversal attempt: ",
				e.what(), "");
			// I guess 403 or 406 is good in this case?
			_status_code = 403;
			build_error_response();
			return;
		}
	}
	if (isDirectory(resolved_path))
	{
		if (request_dir_relative_to_root.length() > 0
			&& request_dir_relative_to_root.at(
				request_dir_relative_to_root.length() - 1) != '/')
		{
			generate_301(request_location_path
				+ request_dir_relative_to_root + '/');
			set_connection_header(request);
			prep_payload();
			print_log("Sent the 301: ", _response_body, "");
			return;
		}
		else
		{
			_status_code = 403;
			print_log("Got PUT request to: ",
				resolved_path.c_str(), " - is a directory");
			build_error_response();
			return;
		}
	}
	if (pathExists(resolved_path)
		&& access(resolved_path.c_str(), W_OK) == -1)
	{
		_status_code = 403;
		print_log("Got PUT request to: ",
			resolved_path.c_str(), " - insufficient permissions");
		build_error_response();
		return;
	}
	file.open(resolved_path.c_str(), std::ios::trunc);
	if (!file.is_open())
	{
		_status_code = 500;
		print_warning("PUT: Couldn't create or write open w/ trunc: ",
			resolved_path.c_str(), "");
		build_error_response();
		return;
	}
	file << request.get_body();
	if (!file.good())
	{
		_status_code = 500;
		print_warning("PUT: Got I/O error while writing to: ",
			resolved_path.c_str(), "");
		build_error_response();
		return;
	}
	file.close();
	generate_204('/' + request_dir_relative_to_root);
	set_connection_header(request);
	prep_payload();
	print_log("Sending the 204:\n", _payload, "\n");
}

void HTTPResponse::generate_301(const std::string &redir_path)
{
	_status_code = 301;
	_headers["Content-Type"] = "plain/text; charset=UTF-8";
	_headers["Location"] = redir_path;
	_response_body = "Moved_permanently to ";
	_response_body += redir_path;
	_response_body += '\n';
}

void HTTPResponse::generate_204(const std::string &content_location)
{
	_status_code = 204;
	_headers["Content-Type"] = "plain/text; charset=UTF-8";
	_headers["Content-Location"] = content_location;
}

int HTTPResponse::find_first_available_index(
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root) const
{
	const std::vector<std::string> *indexes = NULL;
	std::string index_path;

	if (_lp != NULL)
	{
		indexes = &(_lp->getIndexLocation());
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
	// If we receive 1024 (FD limit on our GNU/Linux systems)
	// parallel connections that don't close,
	// then we won't be able to open file descriptors for new connections.
	//
	// We're not obliged to striclty follow the "Connection: keep-alive"
	// request header by RFC, so to prevent this situation from happening
	// we'll close connections after they're served.
	//
	// However, if you'd still like to test the "Connection: keep-alive"
	// model, uncomment the following code and
	// comment the last two lines in this method.
	//
	// The best solution to this though would be to implement
	// a time limit for how long a connection could stay opened.
	// Nevertheless, we're not required to do so by the subject's PDF file.
	/*
	try
	{
		_headers["Connection"] = request.get_header_value(
				"Connection");
	}
	catch (const std::range_error &e)
	{
		_headers["Connection"] = "keep-alive";
	}
	 */
	(void) request;
	_headers["Connection"] = "close";
}

int HTTPResponse::handle_cgi(const HTTPRequest &request,
		std::string &request_dir_root,
		std::string &request_dir_relative_to_root,
		std::string &request_location_path,
		std::string &resolved_path)
{
	time_t current_time;
	pid_t waitpid_code;
	int waitpid_status;
	int child_exit_status;

	if (pipe(_cgi_pipe) == -1)
	{
		print_warning("HTTPResponse::handle_cgi(): pipe() fail", "", "");
		return 500;
	}
	_cgi_launch_time = std::time(NULL);
	if ((_cgi_pid = fork()) == -1)
	{
		print_warning("HTTPResponse::handle_cgi(): fork() fail", "", "");
		(void) close(_cgi_pipe[0]);
		(void) close(_cgi_pipe[1]);
		return 500;
	}
	else if (_cgi_pid == 0)
	{
		(void) request_dir_root;
		(void) close(_cgi_pipe[0]);
		_cgi_pipe[0] = -1;
		this->cgi(request, resolved_path);
	}
	(void) close(_cgi_pipe[1]);
	_cgi_pipe[1] = -1;
	for (;;)
	{
		current_time = std::time(NULL);
		waitpid_code = waitpid(_cgi_pid, &waitpid_status, WNOHANG);
		// waitpid() fail.
		if (waitpid_code == -1)
		{
			print_warning("HTTPResponse::handle_cgi(): waitpid() fail",
				"", "");
			// SIGKILL is better than SIGTERM,
			// since it may kill the process
			// if it's frozen and doesn't respond to SIGTERM.
			(void) kill(_cgi_pid, SIGKILL);
			_cgi_pid = -1;
			(void) close(_cgi_pipe[0]);
			_cgi_pipe[0] = -1;
			return 500;
		}
		// Child exited.
		else if (waitpid_code == _cgi_pid
			&& WIFEXITED(waitpid_status))
		{
			_cgi_pid = -1;
			child_exit_status = WEXITSTATUS(waitpid_status);
			if (child_exit_status != 0)
			{
				(void) close(_cgi_pipe[0]);
				return 502;
			}
			break;
		}
		// Child is running for longer than it's allowed to.
		else if (current_time - _cgi_launch_time > _MAX_CGI_TIME)
		{
			print_warning("HTTPResponse::handle_cgi(): CGI hangup at script: ",
				 request_location_path + request_dir_relative_to_root,
				 "");
			(void) kill(_cgi_pid, SIGKILL);
			_cgi_pid = -1;
			(void) close(_cgi_pipe[0]);
			_cgi_pipe[0] = -1;
			return 504;
		}
	}
	try
	{
		this->copy_child_output_to_payload();
	}
	catch (const std::runtime_error &e)
	{
		print_warning("Couldn't copy child's output to payload: ", e.what(), "");
		(void) close(_cgi_pipe[0]);
		return 500;
	}
	(void) close(_cgi_pipe[0]);
	_cgi_pipe[0] = -1;
	_headers["Connection"] = "close";
	_payload_ready = true;
	return 0;
}

void HTTPResponse::copy_child_output_to_payload()
{
	// 2 KiB buffer to copy child's output to payload.
	enum { BUFFER_SIZE = 2048 };
	char buffer[BUFFER_SIZE];
	ssize_t n;

	for (;;)
	{
		n = read(_cgi_pipe[0], buffer, BUFFER_SIZE);
		if (n == -1)
		{
			throw std::runtime_error(std::string("HTTPResponse::copy_child_output_to_payload(): ")
					+ "read() fail.");
		}
		_payload.append(buffer, static_cast<size_t> (n));
		if (n < BUFFER_SIZE)
		{
			// Finished reading.
			return;
		}
	}
}

void HTTPResponse::cgi(const HTTPRequest &request, std::string &resolved_path)
{
	// To redirect `_response_body` to stdin.
	int redir_stdin[2];
	ssize_t n = 0, written;
	// Actual data required for CGI execution.
	size_t cgi_path_index;
	// std::auto_ptr may be unreliable
	// and std::unique_ptr in unavailable in C++98.
	char ** argv, ** envp;

	if (pipe(redir_stdin) == -1)
	{
		print_err("HTTPResponse::cgi(): pipe() failed", "", "");
		(void) close(_cgi_pipe[1]);
		std::exit(EXIT_FAILURE);
	}
	while (static_cast<size_t> (n) < request.get_body().length())
	{
		written = write(redir_stdin[1], request.get_body().c_str() + n,
				request.get_body().length() -
					static_cast<size_t> (n));
		if (written == -1)
		{
			print_err("HTTPResponse::cgi(): write() failed: ",
				"Couldn't copy _response_body to stdin", "");
			(void) close(_cgi_pipe[1]);
			(void) close(redir_stdin[0]);
			(void) close(redir_stdin[1]);
			std::exit(EXIT_FAILURE);
		}
		n += written;
	}
	(void) close(redir_stdin[1]);
	if (dup2(redir_stdin[0], STDIN_FILENO) == -1
		|| dup2(_cgi_pipe[1], STDOUT_FILENO) == -1)
	{
		print_err("HTTPResponse::cgi(): dup2() failed", "", "");
		(void) close(_cgi_pipe[1]);
		(void) close(redir_stdin[0]);
		std::exit(EXIT_FAILURE);
	}
	(void) close(_cgi_pipe[1]);
	(void) close(redir_stdin[0]);
	// We don't check if std::find() returns `_lp->getCgiExtension().end()`
	// to us, since this method is called by `handle_cgi()`,
	// which in turn should only be called when it's found out
	// that the extension of a file at \p resolved_path
	// belongs to `_lp->_cgi_ext`.
	cgi_path_index = static_cast<size_t> (
			std::find(_lp->getCgiExtension().begin(),
				_lp->getCgiExtension().end(),
				get_file_ext(resolved_path))
			- _lp->getCgiExtension().begin());
	argv = cgi_prep_argv(_lp->getCgiPath().at(cgi_path_index), resolved_path);
	print_log("About to execve() from child. Bye-bye world!", "", "");
	if (argv == NULL)
	{
		print_err("HTTPResponse::cgi(): cgi_prep_argv() failed", "", "");
		std::exit(EXIT_FAILURE);
	}
	else if ((envp = cgi_prep_envp(request)) == NULL)
	{
		print_err("HTTPResponse::cgi(): cgi_prep_envp() failed", "", "");
		this->cgi_free_argv_like_array(argv);
		std::exit(EXIT_FAILURE);
	}
	else if (execve(_lp->getCgiPath().at(cgi_path_index).c_str(),
			argv, envp) == -1)
	{
		print_err("HTTPResponse::cgi(): execve() failed", "", "");
		this->cgi_free_argv_like_array(argv);
		this->cgi_free_argv_like_array(envp);
		std::exit(EXIT_FAILURE);
	}
}

/*
size_t HTTPResponse::cgi_get_path_index(const std::string &resolved_path) const
{
	std::string request_file_ext;
	size_t ret;

	request_file_ext = get_file_ext(resolved_path);
	// It's up to you to ensure that `request_file_ext`
	// exists in `_lp->_cgi_ext`.
	for (ret= 0; ret < _lp->getCgiExtension().size(); ret++)
	{
		const std::string &current_cgi_ext = _lp->getCgiExtension().at(
				ret);

		if (current_cgi_ext.compare(0, request_file_ext.length(),
					request_file_ext) == 0)
		{
			break;
		}
	}
	return ret;
}
 */

char ** HTTPResponse::cgi_prep_argv(const std::string &interpreter_path,
		const std::string &script_path) const
{
	// `interpreter_path`, `script_path` and terminating NULL.
	enum { POINTERS_IN_RET = 3 };
	char ** ret;

	try
	{
		ret = new char * [POINTERS_IN_RET];
	}
	catch (const std::bad_alloc &e)
	{
		return NULL;
	}
	ret[0] = NULL;
	ret[1] = NULL;
	ret[2] = NULL;
	// `interpreter_path`.
	try
	{
		ret[0] = new char [interpreter_path.length() + 1];
	}
	catch (const std::bad_alloc &e)
	{
		delete [] ret;
		return NULL;
	}
	(void) memcpy(ret[0], interpreter_path.c_str(), interpreter_path.length());
	ret[0][interpreter_path.length()] = '\0';
	// `script_path`.
	try
	{
		ret[1] = new char [script_path.length() + 1];
	}
	catch (const std::bad_alloc &e)
	{
		delete [] ret[0];
		delete [] ret;
		return NULL;
	}
	(void) memcpy(ret[1], script_path.c_str(), script_path.length());
	ret[1][script_path.length()] = '\0';
	// Terminating NULL.
	ret[2] = NULL;
	return ret;
}

char ** HTTPResponse::cgi_prep_envp(const HTTPRequest & request) const
{
	// We'll first append all variables to std::vector of std::strings
	// and later convert them to char **.
	std::vector<std::string> vars;
	char client_ip[INET_ADDRSTRLEN];
	std::string header_key;	// For `request`'s header fields.
	char ** ret;

	for (size_t i = 0; environ[i] != NULL; i++)
	{
		vars.push_back(std::string(environ[i]));
	}
	// CGI-specific variables.
	// Not the most elegant solution, but stil...
	// Environment variables for all CGI requests:
	vars.push_back(std::string("SERVER_SOFTWARE=") + SERVER_NAME + "/1.0");
	vars.push_back(std::string("SERVER_NAME=") + SERVER_NAME);
	vars.push_back(std::string("GATEWAY_INERFACE=CGI/1.1"));
	// Environment variables for specific CGI requests:
	vars.push_back(std::string("SERVER_PROTOCOL=HTTP/1.1"));
	vars.push_back(std::string("SERVER_PORT=")
		+ to_string(htons(request.get_server_address().sin_port)));
	switch (request.get_method())
	{
		case HTTPRequest::GET:
			vars.push_back(std::string("REQUEST_METHOD=GET"));
			break;
		case HTTPRequest::POST:
			vars.push_back(std::string("REQUEST_METHOD=POST"));
			break;
		case HTTPRequest::DELETE:
			vars.push_back(std::string("REQUEST_METHOD=DELETE"));
			break;
		case HTTPRequest::PUT:
			vars.push_back(std::string("REQUEST_METHOD=PUT"));
			break;
	}
	vars.push_back(std::string("SCRIPT_NAME=")
		+ request.get_request_path_decoded());
	try
	{
		vars.push_back(std::string("QUERY_STRING=")
			+ request.get_request_query_original());
	}
	catch (const std::runtime_error &e)
	{
		// There is no query in the request.
		vars.push_back(std::string("QUERY_STRING="));
	}
	// Here's a proof of our working implementation of inet_ntop4(),
	// although we believed writing a translator from `in_addr`
	// to it's text form (for example 01111111.00000000.00000000.00000001
	// is "127.0.0.1") is a redundant overkill (and we still do).
	//
	// We surely do know how to translate "127.0.0.1"
	// back to `uint32_t` (typedef of `in_addr`),
	// so please don't mark this w/ -42.
	// We just don't want to implement our own inet_pton(),
	// because we find it pointless by now.
	if (our_inet_ntop4(&(request.get_client_address().sin_addr), client_ip,
		INET_ADDRSTRLEN)
		== NULL)
	{
		print_err("HTTPResponse::cgi_prep_env(): ",
			"our_inet_ntop4() fail", "");
		return NULL;
	}
	vars.push_back(std::string("REMOTE_ADDR=") + client_ip);
	// CONTENT_TYPE and CONTENT_LENGTH.
	// Again, we support only GET and POST for CGI,
	// hence not checking if `request`'s method may be PUT.
	if (request.get_method() == HTTPRequest::POST)
	{
		try
		{
			vars.push_back(std::string("CONTENT_TYPE=")
				+ request.get_header_value("Content-Type"));
		}
		catch (const std::range_error &e)
		{
			print_warning("HTTPResponse::cgi_prep_env(): ",
				"Request method is POST, but \"Content-Type\" isn't set",
				"");
			vars.push_back(std::string("CONTENT_TYPE="));
		}
		try
		{
			vars.push_back(std::string("CONTENT_LENGTH=")
				+ request.get_header_value("Content-Length"));
		}
		catch (const std::range_error &e)
		{
			print_warning("HTTPResponse::cgi_prep_env(): ",
				"Request method is POST, but \"Content-Length\" isn't set",
				"");
			vars.push_back(std::string("CONTENT_LENGTH="));
		}
	}
	// `request`'s header fields.
	for (std::map<std::string, std::string>::const_iterator it = request.get_header_fields().begin();
			it != request.get_header_fields().end(); ++it)
	{
		// In cast of POST, those are already set.
		// In case of GET, they're not needed either way.
		if (it->first == "Content-Type" || it->first == "Content-Length")
		{
			continue;
		}
		header_key = it->first;
		for (size_t i = 0; i < header_key.size(); i++)
		{
			if (std::islower(header_key.at(i)))
			{
				header_key.at(i) = std::toupper(header_key.at(i));
			}
			else if (header_key.at(i) == '-')
			{
				header_key.at(i) = '_';
			}
		}
		header_key.insert(0, "HTTP_");
		vars.push_back(header_key + "=" + it->second);
	}
	// Building "envp".
	try
	{
		// +1 for trailing NULL.
		ret = new char * [vars.size() + 1];
	}
	catch (const std::bad_alloc &e)
	{
		return NULL;
	}
	for (size_t i = 0; i < vars.size(); i++)
	{
		// +1 for '\0' in each string.
		try
		{
			ret[i] = new char [vars.at(i).length() + 1];
		}
		catch (const std::bad_alloc &e)
		{
			for (size_t j = 0; j < i; j++)
			{
				delete [] ret[j];
				delete [] ret;
				return NULL;
			}
		}
		(void) memcpy(ret[i], vars.at(i).c_str(), vars.at(i).length());
		ret[i][vars.at(i).length()] = '\0';
	}
	ret[vars.size()] = NULL;
	return ret;
}

void HTTPResponse::cgi_free_argv_like_array(char ** argv) const
{
	for (size_t i = 0; argv[i] != NULL; i++)
	{
		delete [] argv[i];
	}
	delete [] argv;
}
