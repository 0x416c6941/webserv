#include "../include/HTTPResponse.hpp"

HTTPResponse::HTTPResponse():
        _response_msg(""),
        // _request(NULL),
        // _server_config(NULL),
        _status_code(200),
        _response_ready(false) {}


HTTPResponse::HTTPResponse(const HTTPResponse& other)
	: _response_msg(other._response_msg),
	//   _server_config(other._server_config),
	  _status_code(other._status_code),
	  _response_ready(other._response_ready)
{}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& other) {
	if (this != &other) {
		_response_msg = other._response_msg;
		// _server_config = other._server_config;
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

void HTTPResponse::set_root(const std::string& root)
{
        _root = root;
}

// void HTTPResponse::handleGET(const HTTPRequest& request, const ServerConfig& server_config)
// {
//     // Implement GET handling logic here
//     // For example, read the requested file and set _response_msg
//     // Set _status_code to 200 if successful, or appropriate error code if not
//         void request;
//         void server_config;
// }

void    HTTPResponse::build_response(const ServerConfig& server_config, const HTTPRequest& request)
{
        if(_response_msg.size() > 0)
                _response_msg.clear();
        _response_ready = false;
        // basic validation, probably unnecessary
        if (!validate_request(request)) {
                build_error_response(server_config);
                return;
        }
        HTTPRequest::e_method method = request.get_method();
        set_root(server_config.getRoot());
        switch (method){
                case HTTPRequest::GET:
                        // Handle GET
                        break;

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


// void HTTPResponse::prepare() {
// 	determine_status();
// 	resolve_path();
// 	use_error_page_if_needed();
// 	build_headers();
// }

// void HTTPResponse::determine_status() {
// 	if (!_request.is_complete()) {
// 		_status_code = 400;
// 		_reason_phrase = "Bad Request";
// 		return;
// 	}

// 	if (_request.get_method() != HTTPRequest::GET &&
// 		_request.get_method() != HTTPRequest::POST &&
// 		_request.get_method() != HTTPRequest::DELETE) {
// 		_status_code = 405;
// 		_reason_phrase = "Method Not Allowed";
// 		return;
// 	}

// 	// Additional logic for POST content-length or transfer-encoding
// 	if (_request.get_method() == HTTPRequest::POST) {
// 		if (_request.get_header_value("Content-Length").empty() &&
// 			_request.get_header_value("Transfer-Encoding").empty()) {
// 			_status_code = 411;
// 			_reason_phrase = "Length Required";
// 		}
// 	}
// }

// void HTTPResponse::resolve_path() {
// 	if (_status_code != 200)
// 		return;

// 	_resolved_path = _config.getRoot() + _request.get_request_target();

// 	struct stat sb;
// 	if (stat(_resolved_path.c_str(), &sb) != 0 || S_ISDIR(sb.st_mode)) {
// 		_status_code = 404;
// 		_reason_phrase = "Not Found";
// 		return;
// 	}
// }

// void HTTPResponse::use_error_page_if_needed() {
// 	if (_status_code == 200)
// 		return;

// 	std::map<int, std::string>::const_iterator it = _config.getErrorPages().find(_status_code);
// 	if (it != _config.getErrorPages().end()) {
// 		_resolved_path = _config.getRoot() + it->second;
// 	}
// }

// void HTTPResponse::build_headers() {
// 	_headers["Server"] = "Webserv/1.0";
// 	_headers["Connection"] = "close";
// 	_headers["Content-Type"] = MIME::get_type(_resolved_path);

// 	struct stat sb;
// 	if (stat(_resolved_path.c_str(), &sb) == 0)
// 		_headers["Content-Length"] = std::to_string(sb.st_size);
// }

// std::string HTTPResponse::get_header_string() const {
// 	std::string response = "HTTP/1.1 " + std::to_string(_status_code) + " " + _reason_phrase + "\r\n";
// 	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
// 		response += it->first + ": " + it->second + "\r\n";
// 	}
// 	response += "\r\n";
// 	return response;
// }

// int HTTPResponse::get_status_code() const {
// 	return _status_code;
// }

// std::string HTTPResponse::get_file_path() const {
// 	return _resolved_path;
// }



void    HTTPResponse::reset()
{
        _response_msg.clear();
        _status_code = 200;
        _response_ready = false;
}

