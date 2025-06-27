#include "../include/HTTPResponse.hpp"

HTTPResponse::HTTPResponse()
	: _mime(""),
	  _response_msg(""),
	  _response_body(""),
	  _root(""),
	  _status_code(0),
	  _response_ready(false) {
}

HTTPResponse::HTTPResponse(const HTTPResponse& other)
	: _mime(other._mime),
	  _response_msg(other._response_msg),
	  _response_body(other._response_body),
	  _root(other._root),
	  _status_code(other._status_code),
	  _response_ready(other._response_ready) {
}


HTTPResponse& HTTPResponse::operator=(const HTTPResponse& other) {
	if (this != &other) {
		_mime = other._mime;
		_response_msg = other._response_msg;
		_response_body = other._response_body;
		_root = other._root;
		_status_code = other._status_code;
		_response_ready = other._response_ready;
	}
	return *this;
}


HTTPResponse::~HTTPResponse() {}

void HTTPResponse::set_status_code(int code)
{
        _status_code = code;
}

bool HTTPResponse::is_response_ready() const
{
        return _response_ready;
}

std::string HTTPResponse::get_response_msg() const
{
        return _response_msg;
}


bool HTTPResponse::validate_request(const HTTPRequest& request)
{
    try {
        if (!request.is_complete()) {
            _status_code = 400; // Bad Request
            return false; //should never happen
        }

        HTTPRequest::e_method method = request.get_method();

        // Allow only GET, POST, DELETE
        if (method != HTTPRequest::GET &&
            method != HTTPRequest::POST &&
            method != HTTPRequest::DELETE) {
            _status_code = 405; // Method Not Allowed
            return false;
        }

        // All requests must contain a "Host" header
        try {
            request.get_header_value("Host");
        } catch (const std::out_of_range&) {
            _status_code = 400; // Bad Request (missing Host)
            return false;
        }

        // Additional checks for POST
        // Should we check the body here?
        if (method == HTTPRequest::POST) {
            bool hasContentLength = false;
            bool hasTransferEncoding = false;

            try {
                request.get_header_value("Content-Length");
                hasContentLength = true;
            } catch (...) {}

            try {
                request.get_header_value("Transfer-Encoding");
                hasTransferEncoding = true;
            } catch (...) {}

            if (!hasContentLength && !hasTransferEncoding) {
                _status_code = 411; // Length Required
                return false;
            }
        }

        _status_code = 200; // OK, just in case; it's already 200 by default
    }
    catch (const std::exception& e) {
        std::cerr << "Validation error: " << e.what() << std::endl;
        _status_code = 400;
        return false; // Fallback to Bad Request on unexpected error
    }
    return true; // Request is valid
}

std::string HTTPResponse::get_mime_type(const std::string &path) {
	static std::map<std::string, std::string> mime_map;
	if (mime_map.empty()) {
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
		return "application/octet-stream";

	std::string ext = path.substr(dot);
	for (size_t i = 0; i < ext.size(); ++i) {
		ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));
	}

	std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
	if (it != mime_map.end())
		return it->second;

	return "application/octet-stream";
}

void HTTPResponse::set_root(const std::string& root)
{
        _root = root;
}

bool HTTPResponse::file_exists(const std::string& path) const
{
        struct stat sb;
        return (stat(path.c_str(), &sb) == 0);
}

bool HTTPResponse::is_directory(const std::string& path) const
{
        struct stat sb;
        if (stat(path.c_str(), &sb) != 0)
                return false;
        return S_ISDIR(sb.st_mode);
}


bool HTTPResponse::has_permission(const std::string& path, HTTPRequest::e_method method) const
{
        int mode = 0;

        switch (method) {
                case HTTPRequest::GET:
                        mode = R_OK; // Read permission
                        break;
                case HTTPRequest::POST:
                        mode = W_OK; // Write permission (for appending/uploading)
                        break;
                case HTTPRequest::DELETE:
                        mode = W_OK; // Need write access to the directory (delete op)
                        break;
                default:
                        return false;
        }

        return (access(path.c_str(), mode) == 0);
}

std::string HTTPResponse::resolve_secure_path(const std::string& request_path) const {
	// Step 1: Normalize input
	std::string path = request_path.empty() ? "/" : request_path;

	std::string::size_type pos = path.find_first_of("?#");
	if (pos != std::string::npos)
		path = path.substr(0, pos);

	if (!path.empty() && path[0] == '/')
		path = path.substr(1);

	std::string full_path = _root + "/" + path;

	// Step 2: Canonicalize full_path
	char resolved[PATH_MAX];
	if (!realpath(full_path.c_str(), resolved)) {
		throw HTTPError(404, "resolve_secure_path: unable to resolve file path: " + full_path);
	}
	std::string resolved_path(resolved);

	// Step 3: Canonicalize _root
	char canonical_root[PATH_MAX];
	if (!realpath(_root.c_str(), canonical_root)) {
		throw HTTPError(500, "resolve_secure_path: failed to resolve server root path: " + _root);
	}
	std::string resolved_root(canonical_root);

	// Step 4: Ensure resolved_path is within resolved_root (security check)
	if (resolved_path.compare(0, resolved_root.size(), resolved_root) != 0 ||
	    (resolved_path.size() > resolved_root.size() && resolved_path[resolved_root.size()] != '/')) {
		throw HTTPError(403, "resolve_secure_path: directory traversal attempt detected.");
	}

	return resolved_path;
}




void HTTPResponse::handleGET(const HTTPRequest& request, const ServerConfig& server_config)
{
        const std::vector<Location> loc = server_config.getLocations(); // Unused for now, but should be checked latter
        try {
                std::string requested_path = request.get_request_target();
                std::string full_path = resolve_secure_path(requested_path);


                // If it's a directory, try to append "/index.html"
                if (is_directory(full_path)) {
                        if (!full_path.empty() && full_path[full_path.length() - 1] != '/')
                                full_path += '/';
                        full_path += "index.html";
                }


                // File must exist
                if (!file_exists(full_path)) {
                        _status_code = 404; // Not Found
                        _response_msg = generateErrorPage(_status_code);
                        return;
                }

                 // Must not be a directory (even after index.html fallback)
                if (is_directory(full_path)) {
                        _status_code = 403;
                        _response_msg = generateErrorPage(_status_code);
                        return;
                }

                // Must have read permissions
                if (!has_permission(full_path, HTTPRequest::GET)) {
                        _status_code = 403; // Forbidden
                        _response_msg = generateErrorPage(_status_code);
                        return;
                }

                // Read file contents
                std::ifstream file(full_path.c_str());
                if (!file) {
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
        catch (const HTTPResponse::HTTPError& err) {
	        print_err("Routing error: ", err.message, " (HTTP " + to_string(err.code) + ")");
	        _status_code = err.code;
	        _response_msg = generateErrorPage(_status_code);
                _response_ready = true; // Set response ready even on error
        }
        catch (const std::exception& e) {
                print_err("handleGET error: ", e.what(), "");
	        _status_code = 500;
	        _response_msg = generateErrorPage(_status_code);
                _response_ready = true; // Set response ready even on error
        }
}

std::string HTTPResponse::build_response_msg() const
{
        std::ostringstream response;
        response << "HTTP/1.1 " << _status_code << " " << getReasonPhrase(_status_code) << "\r\n";
        response << "Server: Webserv_hlyshchu_asagymbe\r\n";
        response << "Connection: keep-alive\r\n";
        response << "Content-Type: " << _mime << "\r\n";
        response << "Content-Length: " << _response_body.size() << "\r\n";
        response << "\r\n"; // End of headers
        response << _response_body; // Add the body content
        return response.str();
}

void    HTTPResponse::handle_response_routine(const ServerConfig& server_config, const HTTPRequest& request)
{
        if(_response_msg.size() > 0)
                _response_msg.clear();
        _response_ready = false;
        set_root(server_config.getRoot());
        // basic validation, probably unnecessary
        if (!validate_request(request)) {
                build_error_response(server_config);
                return;
        }
        HTTPRequest::e_method method = request.get_method();
        switch (method){
                case HTTPRequest::GET:
                        handleGET(request, server_config);
                        if (_status_code == 200) {
                                // Build the response msg with headers
                                _response_msg = build_response_msg();
                                _response_ready = true;
                        break;
                        }
                case HTTPRequest::POST:
                        // Handle POST
                        break;

                case HTTPRequest::DELETE:
                        // Handle DELETE
                        break;

                default:
                        // Unknown method (should not occur if validated properly)
                        _status_code = 405;
                        build_error_response(server_config);
                        return;
        }
        return;
}



void HTTPResponse::build_error_response(const ServerConfig& server_config)
{
        std::map<int, std::string>::const_iterator it = server_config.getErrorPages().find(_status_code);
        if (it != server_config.getErrorPages().end()) {
                // If an error page is defined for this status code, use it
                // _response_ready = true;
                }
        else {
                _response_msg = generateErrorPage(_status_code);
                _response_ready = true;
        }
}


void    HTTPResponse::reset()
{
        _response_msg.clear();
        _status_code = 200;
        _response_ready = false;
        _response_body.clear();
        _response_msg.clear();
        _mime.clear();

}

