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
	HTTPResponse(int status_code);
	HTTPResponse(const HTTPResponse& other);
	HTTPResponse& operator=(const HTTPResponse& other);
	~HTTPResponse();
	void			reset();

	void 			build_error_response(const ServerConfig &server_config);
	void 			handle_response_routine(const ServerConfig &server_config, const HTTPRequest &request);
	bool 			is_response_ready() const;
	std::string		get_response_msg() const;

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
	int					_status_code;
	// std::map<std::string, std::string>	_headers;
	std::string				_response_body;
	bool 		  			_response_ready;

	std::string 	get_mime_type(const std::string &path);
	void 		handleGET(const HTTPRequest& request, const ServerConfig& server_config);
	// handlePOST(), handleDELETE().
};
