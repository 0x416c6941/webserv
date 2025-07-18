#pragma once

#include <stdexcept>
#include <netinet/in.h>
#include <vector>
#include <string>
#include <map>
#include <cstddef>

/**
 * A class containing a received and parsed HTTP/1.1 request.
 * Non-standard header fields are also stored, but they're not processed later.
 * Please keep in mind, that this class isn't designed for handling request body.
 * We also intend to support only GET, POST, and DELETE methods.
 */
class HTTPRequest
{
	public:
		HTTPRequest();
		virtual ~HTTPRequest();
		void reset();

		enum e_method
		{
			GET,
			POST,
			DELETE,
			// PUT is not the required by the subject,
			// but is really handsome for receiving files.
			// CGI will be not implemented for it.
			PUT
		};

		class method_not_allowed : public std::exception
		{
			private:
				const std::string _MSG;

			public:
				method_not_allowed(const char * msg);
				method_not_allowed(const std::string &msg);
				virtual ~method_not_allowed() throw();

				virtual const char * what() const throw();
		};

		class http_ver_unsupported : public std::exception
		{
			private:
				const std::string _MSG;

			public:
				http_ver_unsupported(const char * msg);
				http_ver_unsupported(const std::string &msg);
				virtual ~http_ver_unsupported() throw();

				virtual const char * what() const throw();
		};

		class non_ascii_request : public std::exception
		{
			private:
				const std::string _MSG;

			public:
				non_ascii_request(const char * msg);
				non_ascii_request(const std::string &msg);
				virtual ~non_ascii_request() throw();

				virtual const char * what() const throw();
		};

		/**
		 * Set `_server_address` to \p server_address.
		 * @param	server_address	Server's socket address.
		 */
		void set_server_address(const struct sockaddr_in &server_address);

		/**
		 * Get `_server_address`.
		 * @throw	runtime_error	Server's socket address.
		 * 				is not set yet.
		 */
		const struct sockaddr_in &get_server_address() const;

		/**
		 * Set `_client_address` to \p client_address.
		 * @param	client_address	Client's socket address.
		 */
		void set_client_address(const struct sockaddr_in &client_address);

		/**
		 * Get `_client_address`.
		 * @throw	runtime_error	Client's socket address
		 * 				is not set yet.
		 */
		const struct sockaddr_in &get_client_address() const;

		/**
		 * Processes line in \p header_line
		 * until the field terminator ("\r\n") is met:
		 * sets the request method (as well as `_request_target` and `_request_query` fields),
		 * or appends the `_header_fields` with the new field,
		 * or marks the request as complete (fully parsed).
		 * @brief	Process info stored in \p header_line.
		 * @throw	invalid_argument	\p header_line isn't terminated
		 * 					or contains invalid information.
		 * @throw	range_error		Request was fully parsed,
		 * 					yet new information arrived.
		 * @throw	runtime_error		Received a header field
		 * 					whose key was already registered
		 * 					or got the terminating "\r\n"
		 * 					sequence, yet "Host" header field
		 * 					wasn't set.
		 * @throw	method_not_allowed	Got unsupported
		 * 					request method.
		 * @throw	http_ver_unsupported	Request's HTTP version
		 * 					is not supported.
		 * @param	header_line	Header line with information to process.
		 * @return	Processed bytes in \p header_line (including "\r\n").
		 */
		size_t process_header_line(const std::string &header_line);

		/**
		 * Get method of the request.
		 * @throw	runtime_error	Method wasn't set yet.
		 * @return	Method of the request.
		 */
		enum e_method get_method() const;

		/**
		 * Get the request path with some characters possibly encoded.
		 * @throw	runtime_error	Request path wasn't set yet.
		 * @return	Encoded request path.
		 */
		const std::string &get_request_path_original() const;

		/**
		 * Get the request path with possibly encoded characters decoded.
		 * @throw	runtime_error	Request path wasn't set yet.
		 * @return	Decoded request path.
		 */
		const std::string &get_request_path_decoded() const;

		/**
		 * Gets the request path with possibly encoded characters decoded
		 * and stripped of \p loc_path.
		 * @warning	\p loc_path must begin and end with '/'.
		 * @warning	Request path may be a directory.
		 * 		Check if the name either ends with '/'
		 * 		or S_ISDIR() after stat() is true
		 * 		(redirect with 301 in this case).
		 * @throw	runtime_error		Request path
		 * 					wasn't set yet.
		 * @throw	invalid_argument	\p loc_path
		 * 					doesn't start
		 * 					or end with '/'.
		 * @throw	domain_error		Request path
		 * 					doesn't contain
		 * 					\p loc_path.
		 * @param	loc_path	Location path.
		 * @return	Decoded request path stripped of \p loc_path.
		 */
		std::string get_request_path_decoded_strip_location_path(
				const std::string &loc_path) const;

		/**
		 * Get the request query with some characters possibly encoded.
		 * @throw	runtime_error	Request query wasn't set yet.
		 * @return	Encoded request query (empty string if was empty).
		 */
		const std::string &get_request_query_original() const;

		/**
		 * Get the request query with possibly encoded characters decoded.
		 * @throw	runtime_error	Request query wasn't set yet.
		 * @return	Decoded request query (empty string if was empty).
		 */
		const std::string &get_request_query_decoded() const;

		/**
		 * Get the request target with some characters possibly encoded.
		 * @throw	runtime_error	Request target wasn't set yet.
		 * @return	Encoded request target.
		 */
		const std::string &get_request_target() const;

		/**
		 * Get the value of a header with the \p key.
		 * @throw	range_error	Header field with such a key
		 * 				wasn't parsed.
		 * @param	key	Key of the header.
		 * @return	Value of the requested header.
		 */
		const std::string &get_header_value(const std::string &key) const;

		/**
		 * Getter for `_header_fields`.
		 * @throw	runtime_error	Request's isn't fully
		 * 				parsed yet.
		 * @return	`_header_fields`.
		 */
		const std::map<std::string, std::string> &get_header_fields() const;

		/**
		 * Process and save body part stored in \p buffer.
		 * @warning	It's up to you to ensure
		 * 		that request's method is "POST" or "PUT".
		 * @throw	invalid_argument	"Transfer-Encoding" field
		 * 					is set, however \p buffer
		 * 					isn't a complete chunk.
		 * @throw	range_error		Body was already processed.
		 * @throw	runtime_error		Body part in \p buffer
		 * 					is borked.
		 * @throw	domain_error		Neither "Content-Length"
		 * 					nor "Transfer-Encoding"
		 * 					fields are set.
		 * @param	buffer	Body part to process.
		 * @return	Processed bytes in \p buffer.
		 */
		size_t process_body_part(const std::string &buffer);

		/**
		 * Get the body in whatever state it's stored now
		 * (complete or incomplete).
		 * To check if body is fully processed and complete,
		 * use the `is_body_complete()` method.
		 * @return	Request's body.
		 */
		const std::string &get_body() const;

		/**
		 * Check if request's header was fully parsed yet.
		 * @return	true, if yes;
		 * 		false otherwise.
		 */
		bool is_header_complete() const;

		/**
		 * Check if request's body was fully parsed yet.
		 * @warning	If request's method isn't "POST" neither "PUT",
		 * 		presence of "Content-Length"
		 *		or "Transfer-Encoding" wouldn't matter.
		 * @throw	domain_error	Request's method isn't "POST"
		 * 				neither "PUT".
		 * @return	true, if yes;
		 * 		false otherwise.
		 */
		bool is_body_complete() const;

		/*
		 * Check if request was fully parsed yet.
		 * @return	true, if yes;
		 * 		false otherwise.
		 */
		bool is_complete() const;

		/**
 		* Debug function to print all parsed request fields.
 		*/
		void printDebug() const;

	private:
		// It doesn't make sense for `HTTPRequest` to be CopyConstructible
		// or have an ::operator =() available.
		HTTPRequest(const HTTPRequest &src);
		HTTPRequest &operator = (const HTTPRequest &src);

		// Required to determine the port
		// on which request was received
		// and IP of the remote host
		// to set up the CGI environment variables in response later.
		struct sockaddr_in _server_address;
		bool _server_address_is_set;
		struct sockaddr_in _client_address;
		bool _client_address_is_set;

		// All possible information from the start line.
		enum e_method _method;
		bool _method_is_set;
		std::string _request_path_original;
		std::string _request_path_decoded;
		bool _request_path_is_set;
		// Request query is optional.
		std::string _request_query_original;
		std::string _request_query_decoded;
		bool _request_query_is_set;
		// Request path + request query combined together.
		std::string _request_target;
		bool _request_target_is_set;

		// Header fields in format "key:OWS value OWS".
		//
		// All requests must contain a "Host" field.
		// Without it, the server should respond with 400.
		//
		// Additionally, POST method must also include
		// either the "Content-Length" or "Transfer-Encoding" field.
		// If both are present, "Transfer-Encoding" takes precedence.
		// Without neither of those, the server should respond with 411.
		std::map<std::string, std::string> _header_fields;

		bool _header_complete;		// If header's request is fully parsed.

		std::string _body;		// Should be used only in POST methods.
		bool _body_complete;

		/**
		 * Parse method, request target and
		 * optional request query (if present) from \p start_line.
		 * @throw	invalid_argument	The start line stored
		 * 					in \p start_line is malformed.
		 * @throw	http_ver_unsupported	Request's HTTP version
		 * 					is not supported.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t handle_start_line(const std::string &start_line);

		/**
		 * Set the method of the request, depending on the value
		 * stored in \p start_line.
		 * @throw	invalid_argument	The request method
		 * 					isn't supported.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t set_method(const std::string &start_line);

		/**
		 * Parses the `_request_path*`, `_request_query*`
		 * and `_request_target` from \p start_line.
		 * @warning	Only origin form (e.g. "/path")
		 * 		is supported as a `_request_path*`.
		 * @throw	invalid_argument	\p start_line
		 * 					contains
		 * 					invalid information.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @param	pos		Where the request target
		 * 				in \p start_line starts.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t set_request_path_query_and_target(
				const std::string &start_line, size_t pos);

		/**
		 * Parses a request component from \p start_line
		 * beginning at \p pos and ending at \p end.
		 * @warning	If any information is already set in
		 * 		\p component, it will be lost.
		 * @throw	invalid_argument	\p start_line
		 * 					contains
		 * 					invalid information.
		 * @param	comp_original	Where to store
		 * 				encoded component.
		 * @param	comp_decoded	Where to store
		 * 				decoded component.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @param	pos		Where the request component
		 * 				in \p start_line starts.
		 * @param	end		Where does the request component
		 * 				in \p start_line end.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t set_request_component(
				std::string &comp_original, std::string &comp_decoded,
				const std::string &start_line, size_t pos, size_t end);

		/**
		 * Verifies that \p request_path_decoded doesn't contain
		 * any double (or more) consequent slashes.
		 * @param	request_path_decoded	Decoded request path
		 * 					set by `set_request_component()`.
		 * @return	true, if no double (or more) consequent slashes
		 * 			in \p request_path_decoded were found.
		 * @return	false otherwise.
		 */
		bool request_path_decoded_no_double_slash_anywhere(
				const std::string &request_path_decoded) const;

		/**
		 * Decodes the percent-encoded character stored in
		 * \p start_line at \p pos.
		 * @warning	Only ASCII characters are supported.
		 * @throw	invalid_argument	\p start_line
		 * 					contains
		 * 					invalid information.
		 * @throw	range_error		If encoded character
		 * 					can't be stored in `char`
		 * 					(only ASCII is supported by us).
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @param	pos		Where the percent-encoded
		 * 				character starts
		 * 				(also an output parameter
		 * 				where it ends).
		 * @return	Decoded character.
		 */
		char decode_percent_encoded_character(const std::string &start_line,
				size_t &pos);

		/**
		 * Parse header field from \p header_field
		 * and store the result in `_header_fields`.
		 * @throw	invalid_argument	The header field stored
		 * 					in \p header_field is malformed.
		 * @throw	runtime_error		Received a header field
		 * 					whose key was already registered.
		 * @param	header_field	Header field with the "\r\n" erased.
		 * @return	Processed bytes in \p header_field.
		 */
		size_t handle_header_field(const std::string &header_field);

		/**
		 * Process and save body part stored in \p buffer.
		 * If got all bytes specified in "Content-Length",
		 * sets `_body_complete` to true.
		 * @warning	Call this method only if "Content-Length" field is set.
		 * @throw	range_error	Body was already processed.
		 * @throw	runtime_error	"Content-Length" doesn't contain
		 * 				a valid number.
		 * @param	buffer	Body part to process.
		 * @return	Processed bytes in \p buffer.
		 */
		size_t process_body_part_cl(const std::string &buffer);

		/**
		 * Process and save body part stored in \p buffer.
		 * If got chunk with size 0, sets `_body_complete` to true.
		 * @warning	Call this method only if "Transfer-Encoding" field
		 * 		is set to "chunked".
		 * @warning	Only "chunked" method is supported.
		 * @throw	invalid_argument	"Transfer-Encoding" field
		 * 					is set to "chunked",
		 * 					however \p buffer
		 * 					isn't a complete chunk.
		 * @throw	range_error		Body was already processed.
		 * @throw	runtime_error		Body part in \p buffer
		 * 					is borked.
		 * @param	buffer	Body part to process.
		 * @return	Processed bytes in \p buffer.
		 */
		size_t process_body_part_te(const std::string &buffer);
};
