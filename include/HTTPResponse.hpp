#pragma once
#include "Webserv.hpp"
#include "HTTPRequest.hpp"
#include "ServerConfig.hpp"
#include "MIME.hpp"

/**
 * @brief The HTTPResponse class is responsible for constructing a 
 * HTTP response message (status line, headers, and body) 
 * based on the request and server configuration.
 * 
 */
class HTTPResponse {
public:
	HTTPResponse();
	// HTTPResponse(const HTTPRequest& request, const ServerConfig& config);
	
	~HTTPResponse();
	
	
	// void prepare();
	// std::string get_header_string() const;
	// int get_status_code() const;
	// std::string get_file_path() const;
	
	
	void 			set_status_code(int code);
	void 			build_error_response();
	bool 			is_response_ready() const;
	std::string 		get_response_msg() const;
	
	void			reset();
	
private:
	HTTPResponse(const HTTPResponse& other); 
	HTTPResponse& operator=(const HTTPResponse& other); 
	std::string 		_response_msg;
	// const HTTPRequest*      _request;
	const ServerConfig*     _server_config;

	int                     _status_code;
	bool 		  	_response_ready;
	// std::map<std::string, std::string> _headers;
	// std::string             _resolved_path;

	// void determine_status();
	// void resolve_path();
	// void build_headers();
	// void use_error_page_if_needed();
};
