#pragma once
#include "Webserv.hpp"
#include "HTTPRequest.hpp"
#include "ServerConfig.hpp"

/**
 * @brief The HTTPResponse class is responsible for constructing a
 * HTTP response message (status line, headers, and body)
 * based on the request and server configuration.
 */
class HTTPResponse {
public:
	HTTPResponse();
	HTTPResponse(const HTTPResponse& other);
	// HTTPResponse(const HTTPRequest& request, const ServerConfig& config);
	HTTPResponse& operator=(const HTTPResponse& other);
	~HTTPResponse();


	// void prepare();
	// std::string get_header_string() const;
	// int get_status_code() const;
	// std::string get_file_path() const;

	void 			set_root(const std::string& root);
	void 			set_status_code(int code);
	void 			build_error_response(const ServerConfig& server_config);
	bool 			is_response_ready() const;
	std::string 		get_response_msg() const;
	void 			handle_response_routine(const ServerConfig& server_config, const HTTPRequest& request);
	void			reset();

public:
	class HTTPError : public std::exception {
		public:
			// Those shouldn't be public...
			int code;
			std::string message;

			HTTPError(int status_code, const std::string &msg);
			virtual ~HTTPError() throw();

			virtual const char* what() const throw();
	};


private:
	std::string 				_mime;
	std::string 				_response_msg;
	std::string 				_response_body;
	std::string 				_root;
	int                     		_status_code;
	bool 		  			_response_ready;
	// std::map<std::string, std::string> _headers;
	// std::string             _resolved_path;

	// helpers
	bool 		has_permission(const std::string& path, HTTPRequest::e_method method) const;
	std::string 	resolve_secure_path(const std::string& request_path) const;

	/**
 	* @brief Validates an HTTP request according to HTTP/1.1 protocol rules.
 	*
 	* This function checks the structural validity and completeness of the request.
 	* It ensures that:
 	* - The request is fully parsed.
 	* - The "Host" header is present (as required by HTTP/1.1).
 	* - For POST requests, either "Content-Length" or "Transfer-Encoding" is provided.
 	*
 	* If the request is invalid, the appropriate HTTP status code is set:
 	* - 400 Bad Request: if the request is incomplete or malformed.
 	* - 405 Method Not Allowed: if the method is not supported.
 	* - 411 Length Required: if a POST request lacks both length and transfer encoding headers.
 	*
 	* @param request A constant reference to the HTTPRequest to be validated.
 	* @return true if the request is valid and all required headers/methods are satisfied; false otherwise.
	*/
	bool		validate_request(const HTTPRequest& request);

	std::string 	build_response_msg() const;
	std::string 	get_mime_type(const std::string &path);
	void 		handleGET(const HTTPRequest& request, const ServerConfig& server_config);
};
