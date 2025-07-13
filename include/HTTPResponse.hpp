#pragma once

#include "ServerConfig.hpp"
#include "HTTPRequest.hpp"
#include <map>
#include <string>
#include <stdexcept>

/**
 * The HTTPResponse class is responsible for constructing a
 * HTTP response message (status line, headers, and body)
 * based on the request and server configuration.
 */
class HTTPResponse
{
	public:
		HTTPResponse();
		HTTPResponse(int status_code);
		HTTPResponse(const HTTPResponse &other);
		HTTPResponse &operator= (const HTTPResponse &other);
		~HTTPResponse();

		class directory_traversal_detected : public std::exception
		{
			private:
				const char * m_msg;

			public:
				directory_traversal_detected(const char * msg);
				directory_traversal_detected(const std::string &msg);

				const char * what() const throw();
		};

		/**
		 * Set the `_server_cfg`.
		 * @param	server_cfg	New value for `_server_cfg`.
		 */
		void		set_server_cfg(ServerConfig *server_cfg);

		/**
		 * Build an error response based on `_status_code`.
		 * @throw	runtime_error	`_server_cfg` wasn't set
		 * 				or response is already prepared.
		 */
		void 		build_error_response();

		/**
		 * Handle \p request and generate response to it.
		 * @throw	runtime_error	`_server_cfg` wasn't set
		 * 				or response is already prepared.
		 * @param	request	Request to handle.
		 */
		void 		handle_response_routine(const HTTPRequest &request);

		/**
		 * Is response ready?
		 * @return	true, if yes.
		 * @return	false, if not.
		 */
		bool 		is_response_ready() const;

		/**
		 * Get the response to be sent with send().
		 * @throw	runtime_error	Response isn't ready yet.
		 * @return	Response ready to be sent with send().
		 */
		std::string	get_response_msg();

		/**
		 * Check if connection should be closed or not.
		 * This will be the case if we're sending either
		 * an error response,
		 * or if we got "Connection: close" header in the request.
		 * @throw	runtime_error	Response isn't ready yet.
		 * @return	true, if connection should be closed
		 * 		after response is set.
		 * @return	false, if connection should NOT be closed
		 * 		after response is set.
		 */
		bool		should_close_connection() const;

	private:
		ServerConfig				*_server_cfg;
		int					_status_code;
		std::map<std::string, std::string>	_headers;
		std::string				_response_body;
		bool 		  			_response_ready;

		std::string 	get_mime_type(const std::string &path);

		/**
		 * Handles the "GET" method:
		 * sets the `_status_code`, required headers in `_headers`
		 * and reads the requested file to `_response_body`.
		 *
		 * If any error is encountered,
		 * respective error response is built.
		 * @brief	Handle the "GET" request method.
		 * @param	request	Request to handle.
		 */
		void 		handle_get(const HTTPRequest& request);

		// handle_post(), handle_delete().

		/**
		 * Appends "Server" and "Content-Length" headers
		 * to `_headers`.
		 * @warning	"Content-Type" and "Connection" headers
		 * 		must be set in build_error_response() or
		 * 		handle_response_routine() respective methods.
		 */
		void		append_required_headers();

		/**
		 * Resolves the request path by concatenating
		 * \p root and \p request_relative_path
		 * while detecting directory traversal.
		 * @throw	directory_traversal_detected	Detected
		 * 						an attempt
		 * 						of directory
		 * 						traversal.
		 * @throw	std::invalid_argument		\p
		 * 						request_relative_path
		 * 						starts
		 * 						with '/'.
		 * @param	root			Location or server root
		 * 					(may be relative).
		 * @param	request_relative_path	Request path relative
		 * 					to \p root (must NOT
		 * 					start with '/').
		 * @return	Resolved request path.
		 */
		std::string	resolve_path(const std::string &root,
				const std::string &request_relative_path) const;
};
