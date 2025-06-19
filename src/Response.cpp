#include "../include/Response.hpp"

HTTPResponse::HTTPResponse(const HTTPRequest& request, const ServerConfig& config)
: _request(request), _config(config), _status_code(200), _reason_phrase("OK") {}

HTTPResponse::~HTTPResponse() {}

void HTTPResponse::prepare() {
	determine_status();
	resolve_path();
	use_error_page_if_needed();
	build_headers();
}

void HTTPResponse::determine_status() {
	if (!_request.is_complete()) {
		_status_code = 400;
		_reason_phrase = "Bad Request";
		return;
	}

	if (_request.get_method() != HTTPRequest::GET &&
		_request.get_method() != HTTPRequest::POST &&
		_request.get_method() != HTTPRequest::DELETE) {
		_status_code = 405;
		_reason_phrase = "Method Not Allowed";
		return;
	}

	// Additional logic for POST content-length or transfer-encoding
	if (_request.get_method() == HTTPRequest::POST) {
		if (_request.get_header_value("Content-Length").empty() &&
			_request.get_header_value("Transfer-Encoding").empty()) {
			_status_code = 411;
			_reason_phrase = "Length Required";
		}
	}
}

void HTTPResponse::resolve_path() {
	if (_status_code != 200)
		return;

	_resolved_path = _config.getRoot() + _request.get_request_target();

	struct stat sb;
	if (stat(_resolved_path.c_str(), &sb) != 0 || S_ISDIR(sb.st_mode)) {
		_status_code = 404;
		_reason_phrase = "Not Found";
		return;
	}
}

void HTTPResponse::use_error_page_if_needed() {
	if (_status_code == 200)
		return;

	std::map<int, std::string>::const_iterator it = _config.getErrorPages().find(_status_code);
	if (it != _config.getErrorPages().end()) {
		_resolved_path = _config.getRoot() + it->second;
	}
}

void HTTPResponse::build_headers() {
	_headers["Server"] = "Webserv/1.0";
	_headers["Connection"] = "close";
	_headers["Content-Type"] = MIME::get_type(_resolved_path);

	struct stat sb;
	if (stat(_resolved_path.c_str(), &sb) == 0)
		_headers["Content-Length"] = std::to_string(sb.st_size);
}

std::string HTTPResponse::get_header_string() const {
	std::string response = "HTTP/1.1 " + std::to_string(_status_code) + " " + _reason_phrase + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response += it->first + ": " + it->second + "\r\n";
	}
	response += "\r\n";
	return response;
}

int HTTPResponse::get_status_code() const {
	return _status_code;
}

std::string HTTPResponse::get_file_path() const {
	return _resolved_path;
}
