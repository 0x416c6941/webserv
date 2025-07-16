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
		void			set_server_cfg(ServerConfig *server_cfg);

		/**
		 * Build an error response based on `_status_code`.
		 * @throw	runtime_error	`_server_cfg` wasn't set
		 * 				or response is already prepared.
		 */
		void 			build_error_response();

		/**
		 * Handle \p request and generate response to it.
		 * @throw	runtime_error	`_server_cfg` wasn't set
		 * 				or response is already prepared.
		 * @param	request	Request to handle.
		 */
		void 			handle_response_routine(
				const HTTPRequest &request);

		/**
		 * Getter for `_payload_ready`.
		 * @return	`_payload_ready` value.
		 */
		bool 			is_response_ready() const;

		/**
		 * Get the response to be sent with send().
		 * @throw	runtime_error	Response isn't ready yet.
		 * @return	Response ready to be sent with send().
		 */
		const std::string	&get_response_msg() const;

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
		bool			should_close_connection() const;

	private:
		ServerConfig				*_server_cfg;
		int					_status_code;
		std::map<std::string, std::string>	_headers;
		std::string				_response_body;
		// `_status_code` + `_headers` + `_response_body` combined.
		std::string				_payload;
		// `_payload_ready` should only be set to true
		// in `prep_payload()`.
		// Don't set it manually.
		bool 		  			_payload_ready;

		// Pointer to Location corresponding to request
		// to process received in `handle_response_routine()`.
		// If set to NULL, `_server_cfg` ought to be used instead.
		const Location				*_lp;

		/**
		 * Gets the mime type depending on extension
		 * of the file in \p path.
		 * @param	path	Path to a file.
		 * @return	Appropriate mime type for file in \p path.
		 */
		std::string 	get_mime_type(const std::string &path);

		/**
		 * Prepares `_payload` by combining
		 * `_status_code`, `_headers` and `_response_body`.
		 * `_headers` shall also be appended
		 * with `append_required_headers()`.
		 * @throw	runtime_error	Payload is already prepared.
		 */
		void		prep_payload();

		/**
		 * Appends "Server" and "Content-Length" headers
		 * to `_headers`.
		 * @warning	"Content-Type" and "Connection" headers
		 * 		must be set in build_error_response() or
		 * 		handle_response_routine() respective methods.
		 */
		void		append_required_headers();

		/**
		 * Handles the "GET" method:
		 * sets the `_status_code`, required headers in `_headers`
		 * and reads the requested file to `_response_body`.
		 *
		 * If any error is encountered,
		 * respective error response is built.
		 * @brief	Handle the "GET" request method.
		 * @warning	Parameters' validity isn't checked.
		 * 		It's up to the user to ensure their validity.
		 * @param	request				Request to handle.
		 * 						in \p request.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @param	request_location_path		`getPath()` from `_lp`
		 * 						or "/" if `_lp` is NULL.
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if p lp is NULL)
		 * 						with trailing '/'.
		 * @param	resolved_path			\p request_dir_root
		 * 						concatenated with
		 * 						\p request_dir_relative_to_root
		 * 						given that it's not
		 * 						a directory traversal
		 * 						attempt.
		 */
		void 		handle_get(const HTTPRequest& request,
				std::string &request_dir_relative_to_root,
				std::string &request_location_path,
				std::string &request_dir_root,
				std::string &resolved_path);

		// handle_post(), handle_delete(), handle_put().

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

		/**
		 * We don't have to support custom return pages.
		 * To make things easier, let's just generate them in code.
		 * @brief	Generate 301 page ("Content-Type"
		 * 		will be "plain/text; charset=UTF-8").
		 * 		Sets `_status_code`, "Location" header
		 * 		and `_response_body`.
		 * @param	redir_path	Redirection path.
		 */
		void		generate_301(const std::string &redir_path);

		/**
		 * Iterate through available indexes in `_lp` or
		 * `_server_cfg` (if \p lp is NULL)
		 * and append the first available to
		 * \p request_dir_relative_to_root
		 * (only if that index's file name contains
		 * file name of \p request_dir_relative_to_root in itself).
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if `_lp` is NULL)
		 * 						with trailing '/'.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @return	0, if got some index that exists and is readable
		 * 		and that was appended to
		 * 		\p request_dir_relative_to_root.
		 * @return	-1, if no such index was found.
		 */
		int		find_first_available_index(
				std::string &request_dir_root,
				std::string &request_dir_relative_to_root) const;

		/**
		 * Generate directory listing page
		 * with files and directories at \p path (excluding . and ..).
		 * "Content-Type" will be "text/html"
		 * and `_status_code` will be 200.
		 * @throw	std::ios_base::failure	Got IO error.
		 * @param	dir	Directory to list files
		 * 			and directories in.
		 */
		void		generate_auto_index(const std::string &path);

		/**
		 * Sets the "Connection" header in `_headers`.
		 * Basically tries to copy it from \p request,
		 * however if it's missing there, sets it to "keep-alive".
		 * @param	request		Request to handle.
		 */
		void		set_connection_header(const HTTPRequest &request);
};
