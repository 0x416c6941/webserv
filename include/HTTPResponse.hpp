#pragma once

#include "ServerConfig.hpp"
#include "HTTPRequest.hpp"
#include <map>
#include <string>

/**
 * The HTTPResponse class is responsible for constructing a
 * HTTP response message (status line, headers, and body)
 * based on the request and server configuration.
 */
class HTTPResponse
{
	public:
		HTTPResponse(ServerConfig *server_cfg);
		HTTPResponse(ServerConfig *server_cfg, int status_code);
		HTTPResponse(const HTTPResponse &other);
		HTTPResponse &operator= (const HTTPResponse &other);
		~HTTPResponse();

		/*
		class HTTPError : public std::exception {
			public:
				// Those shouldn't be public,
				// however some code already relies
				// on those fields being so.
				int code;
				std::string message;

				HTTPError(int status_code, const std::string &msg);
				virtual ~HTTPError() throw();

				virtual const char* what() const throw();
		};
	 	*/

		void 		build_error_response();
		void 		handle_response_routine(const HTTPRequest &request);
		bool 		is_response_ready() const;
		std::string	get_response_msg() const;

	private:
		ServerConfig				*_server_cfg;
		int					_status_code;
		std::map<std::string, std::string>	_headers;
		std::string				_response_body;
		bool 		  			_response_ready;

		std::string 	get_mime_type(const std::string &path);

		void 		handleGET(const HTTPRequest& request);
		// handlePOST(), handleDELETE().

		// We don't want a default constructor to be available.
		HTTPResponse();
};
