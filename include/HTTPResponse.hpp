#pragma once

#include <stdexcept>
#include "ServerConfig.hpp"
#include "HTTPRequest.hpp"
#include <string>
#include <map>
#include <sys/types.h>
#include <ctime>

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
				const std::string _MSG;

			public:
				directory_traversal_detected(const char * msg);
				directory_traversal_detected(const std::string &msg);
				virtual ~directory_traversal_detected() throw();

				virtual const char * what() const throw();
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
		void 			handle_response_routine(const HTTPRequest &request);

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
		// `_status_code` + `_headers` + `_response_body` combined,
		// if request's path isn't CGI.
		//
		// If request's path is CGI, then child's output.
		std::string				_payload;
		// `_payload_ready` should only be set to true
		// in `prep_payload()`.
		// Don't set it manually.
		bool 		  			_payload_ready;

		// Pointer to Location corresponding to request
		// to process received in `handle_response_routine()`.
		// If set to NULL, `_server_cfg` ought to be used instead.
		const Location				*_lp;

		// CGI-related data.
		pid_t					_cgi_pid;
		int					_cgi_pipe[2];
		time_t					_cgi_launch_time;
		// Time in seconds for maximum CGI execution duration.
		// If CGI doesn't finish execution within this time,
		// it will be killed and 504 will be returned.
		static const time_t			_MAX_CGI_TIME = 10;

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
		 * Handles the "GET" method:
		 * sets the `_status_code`, required headers in `_headers`,
		 * reads the requested file to `_response_body`
		 * (or, if it's CGI, launches it)
		 * and generates the response to `_payload`.
		 *
		 * If any error is encountered,
		 * respective error response is built.
		 * @brief	Handle the "GET" request method.
		 * @warning	Parameters' validity isn't checked.
		 * 		It's up to the user to ensure their validity.
		 * @param	request				Request to handle.
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if p lp is NULL)
		 * 						with trailing '/'.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @param	request_location_path		`getPath()` from `_lp`
		 * 						or "/" if `_lp` is NULL.
		 * @param	resolved_path			\p request_dir_root
		 * 						concatenated with
		 * 						\p request_dir_relative_to_root
		 * 						given that it's not
		 * 						a directory traversal
		 * 						attempt.
		 */
		void 		handle_get(const HTTPRequest &request,
				std::string &request_dir_root,
				std::string &request_dir_relative_to_root,
				std::string &request_location_path,
				std::string &resolved_path);

		/**
		 * Handles the "POST" method:
		 * sets the `_status_code`, required headers in `_headers`,
		 * and generates the response to `_payload`.
		 *
		 * If the requested path if CGI script,
		 * launches it with the received data,
		 * otherwise appends the data to existing file.
		 * If file doesn't exist, data simply get discarded
		 * (and 204 will be returned).
		 *
		 * If any error is encountered,
		 * respective error response is built.
		 * @brief	Handle the "POST" request method.
		 * @warning	Parameters' validity isn't checked.
		 * 		It's up to the user to ensure their validity.
		 * @param	request				Request to handle.
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if p lp is NULL)
		 * 						with trailing '/'.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @param	request_location_path		`getPath()` from `_lp`
		 * 						or "/" if `_lp` is NULL.
		 * @param	resolved_path			\p request_dir_root
		 * 						concatenated with
		 * 						\p request_dir_relative_to_root
		 * 						given that it's not
		 * 						a directory traversal
		 * 						attempt.
		 */
		void		handle_post(const HTTPRequest &request,
				std::string &request_dir_root,
				std::string &request_dir_relative_to_root,
				std::string &request_location_path,
				std::string &resolved_path);

		/**
		 * Handles the "DELETE" method:
		 * sets the `_status_code`, required headers in `_headers`,
		 * and generates the response to `_payload`.
		 *
		 * If the requested path is a directory
		 * and doesn't end with '/', client will be redirected
		 * to the same path but with '/' at the end.
		 * If the requested path is a directory and ends with '/',
		 * 403 will be returned (std::remove() can only delete files
		 * and we don't have any available syscall
		 * to delete directories).
		 *
		 * If the requested path doesn't exist, 404 will be returned.
		 * If the requested path does exist but couldn't be deleted
		 * due to permission denial (errno is EACCES),
		 * 403 will be returned.
		 * If the requested path does exist but couldn't be deleted
		 * due to any other reason, 500 will be returned.
		 *
		 * If the requested path exists and was deleted,
		 * 204 will be returned.
		 * @brief	Handle the "POST" request method.
		 * @warning	Parameters' validity isn't checked.
		 * 		It's up to the user to ensure their validity.
		 * @param	request				Request to handle.
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if p lp is NULL)
		 * 						with trailing '/'.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @param	request_location_path		`getPath()` from `_lp`
		 * 						or "/" if `_lp` is NULL.
		 * @param	resolved_path			\p request_dir_root
		 * 						concatenated with
		 * 						\p request_dir_relative_to_root
		 * 						given that it's not
		 * 						a directory traversal
		 * 						attempt.
		 */
		void		handle_delete(const HTTPRequest &request,
				std::string &request_dir_root,
				std::string &request_dir_relative_to_root,
				std::string &request_location_path,
				std::string &resolved_path);


		// TODO: handle_put().

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
		 * Generate the "No Content" headers
		 * ("Content-Type" and "Content-Location")
		 * and set `_status_code`.
		 * @param	content_location	"Content-Location"
		 * 					header's value.
		 */
		void		generate_204(const std::string &content_location);

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

		/**
		 * Handles CGI \p request with fork().
		 *
		 * Main process will set up pipes
		 * and wait at maximum for `_MAX_CGI_TIME` seconds
		 * for child (CGI) process to finish execution.
		 *
		 * Child process will redirect `_request_body` to it's stdin,
		 * stdout to `_cgi_pipe[1]`,
		 * and execve() into the the CGI handler
		 * with the request path as argument for processing.
		 * All CGI-specific environment variables
		 * will also be set up.
		 *
		 * If any syscall in main or child process fails,
		 * 500 will be returned.
		 *
		 * If child doesn't finish execution within
		 * it's maximum allowed execution time,
		 * it will be killed and method will return 504.
		 *
		 * If child does finish execution within this time
		 * and exits with return code 0,
		 * "Connection" header in `_headers` will be set to "close"
		 * and `_payload` will contain the data generated by that CGI.
		 *
		 * If child returns with exit code other than zero
		 * (or CGI handler can't be launched / doesn't exist),
		 * 502 will be returned.
		 * @warning	It's up to you to ensure \p resolved_path
		 * 		exists as a regular file and can be read.
		 * @warning	It's up to you to ensure that extension
		 * 		of \p resolved_path is registered
		 * 		as CGI extension in `_lp`.
		 * @warning	Calling this method when `_lp` is NULL
		 * 		will result in segfault!
		 * @brief	Handles CGI \p request.
		 * @param	request				Request to handle.
		 * @param	request_dir_root		Root or alias
		 * 						of `_lp`
		 * 						or `_server->Root()`
		 * 						(if p lp is NULL)
		 * 						with trailing '/'.
		 * @param	request_dir_relative_to_root	Request path to file
		 * 						or directory
		 * 						in \p request_dir_root.
		 * @param	request_location_path		`getPath()` from `_lp`
		 * 						or "/" if `_lp` is NULL.
		 * @param	resolved_path			\p request_dir_root
		 * 						concatenated with
		 * 						\p request_dir_relative_to_root
		 * 						given that it's not
		 * 						a directory traversal
		 * 						attempt.
		 * @return	0, if everything went alright.
		 * 		In that case, response is prepared.
		 * @return	Any other value than 0
		 * 		signals error code which should be later used
		 * 		for building error response.
		 */
		int		handle_cgi(const HTTPRequest &request,
				std::string &request_dir_root,
				std::string &request_dir_relative_to_root,
				std::string &request_location_path,
				std::string &resolved_path);

		/**
		 * Copies `_cgi_pipe[0]` to `_payload`.
		 * @throw	runtime_error	read() fail.
		 */
		void		copy_child_output_to_payload();

		/**
		 * Child routine of executing CGI in \p request.
		 * Redirect `_request_body` to stdin, stdout to `_cgi_pipe[1]`,
		 * and execve() into the the CGI handler.
		 * All CGI-specific environment variables
		 * will also be set up.
		 *
		 * If execve() fails, exit()'s with "EXIT_FAILURE".
		 * @warning	This method never returns.
		 * @param	request		Request to handle.
		 * @param	resolved_path	Path to the script to process
		 * 				with CGI.
		 */
		void		cgi(const HTTPRequest &request,
				std::string &resolved_path);

		/**
		 * Get index of extension of \p resolved_path in
		 * `_lp->_cgi_ext`.
		 * This is needed for `cgi()` to determine
		 * the index of CGI interpreter.
		 * @warning	It's up to you to ensure that
		 * 		`request_file_ext` exists in `_lp->cgi_ext`.
		 * 		If it doesn't, no exception will be thrown
		 * 		and index in `_lp->_cgi_path` will be invalid.
		 * @param	resolved_path	Path to the script to process
		 * 				with CGI.
		 */
		/*
		size_t		cgi_get_path_index(
				const std::string &resolved_path) const;
		 */

		/**
		 * Returns an "argv"-like array (that is NULL-terminated)
		 * of \p interpreter_path and \p script_path
		 * required for `cgi()` to execve() into interpreter.
		 * @param	interpreter_path	Path to interpreter
		 * 					for \p script_path.
		 * @param	script_path		Path to the script
		 * 					to process with CGI.
		 * @return	"argv-like" array of \p interpreter_path
		 * 		and \p script_path on success.
		 * @return	NULL on some failure.
		 */
		char **		cgi_prep_argv(
				const std::string &interpreter_path,
				const std::string &script_path) const;

		/**
		 * Returns an "argv"-like array (that is NULL-terminated)
		 * of `environ` and additionally
		 * CGI-specific envrionment variables.
		 * @param	request	CGI request to handle.
		 * @return	"argv-like" array of `environ`
		 * 		and additionally CGI-specific
		 * 		environment variables on success.
		 * @return	NULL on some failure.
		 */
		char **		cgi_prep_envp(const HTTPRequest & request) const;

		/**
		 * Helper for `cgi()` to free() \p arr
		 * in case of a system error.
		 * @param	arr	"argv"-like (NULL-terminated)
		 * 			array of C strings.
		 */
		void		cgi_free_argv_like_array(char ** arr) const;
};
