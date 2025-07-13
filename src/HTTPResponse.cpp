#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"
#include <cstddef>
#include <stdexcept>
#include <map>
#include "Webserv.hpp"

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

/*
void    HTTPResponse::handle_response_routine(const ServerConfig& server_config, const HTTPRequest& request)
{
	if (_response_msg.size() > 0)
	{
		_response_msg.clear();
	}
	_response_ready = false;
	set_root(server_config.getRoot());
	// Basic validation, probably unnecessary.
	if (!validate_request(request)) {
		build_error_response(server_config);
		return;
	}
	HTTPRequest::e_method method = request.get_method();
	switch (method) {
		case HTTPRequest::GET:
			handleGET(request, server_config);
			// TODO: What if _status_code isn't 200?...
			if (_status_code == 200)
			{
				// Build the response msg with headers.
				_response_msg = build_response_msg();
				_response_ready = true;
			}
			break;
		case HTTPRequest::POST:
			// Handle POST.
			_status_code = 204;
			_response_msg = build_response_msg();
			_response_ready = true;
			break;
		case HTTPRequest::DELETE:
			// Handle DELETE.
			break;
		default:
			// Unknown method (should not NEVER happen).
			_status_code = 405;
			build_error_response(server_config);
			return;
	}
}
 */
// TODO: Temporary code.
void HTTPResponse::handle_response_routine(const HTTPRequest &request)
{
	(void) request;

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
	_status_code = 501;
	this->build_error_response();
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
	else if (_status_code >= 400	// 4xx and 5xx are error codes.
		|| (_headers.find("Connection") != _headers.end()
			&& _headers.at("Connection").compare("close") == 0))
	{
		return true;
	}
	return false;
}

/*
HTTPResponse::HTTPError::HTTPError(int status_code, const std::string &msg)
	: code(status_code), message(msg)
{
}

HTTPResponse::HTTPError::~HTTPError() throw()
{
}

const char *HTTPResponse::HTTPError::what() const throw()
{
	return message.c_str();
}

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

/*
void HTTPResponse::handleGET(const HTTPRequest& request, const ServerConfig& server_config)
{
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
}
 */

void HTTPResponse::append_required_headers()
{
	_headers["Server"] = "webserv of hlyshchu and asagymba";
	_headers["Content_Length"] = to_string(_response_body.length());
}
