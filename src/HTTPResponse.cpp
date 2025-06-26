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

void    HTTPResponse::build_response(const ServerConfig& server_config)
{
        _response_msg.clear();
        _response_ready = false;
        (void) server_config;
        // if (_status_code >= 400) {
        //         build_error_response();
        // } else {
        //         // Here you would build a normal response based on the request
        //         // For now, we just set a placeholder message
        //         _response_msg = "HTTP/1.1 " + to_string(_status_code) + " OK\r\n\r\n";
        //         _response_ready = true;
        // }
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

