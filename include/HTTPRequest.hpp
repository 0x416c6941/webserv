#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstddef>
#include <iostream> //for debug
#include <iomanip> //for debug

//large_client_header_buffers -> We don't store it in server config so we can use "magix_number" here
// protection against ddos + we 

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
		void 		reset();
		enum e_method
		{
			GET,
			POST,
			DELETE
		};

		/**
		 * Processes line in \p info
		 * until the field terminator ("\r\n") is met:
		 * sets the request method (as well as `_request_target` and `_request_query` fields),
		 * or appends the `_header_fields` with the new field,
		 * or marks the request as complete (fully parsed).
		 * @brief	Process info stored in \p info.
		 * @throw	invalid_argument	\p info isn't terminated
		 * 					or contains invalid information.
		 * @throw	range_error		Request was fully parsed,
		 * 					yet new information arrived.
		 * @param	info	Line with information to process.
		 * @return	Processed bytes in \p info (including "\r\n").
		 */
		size_t process_info(const std::string &info);

		/**
		 * Get method of the request.
		 * @throw	runtime_error	Method wasn't set yet.
		 * @return	Method of the request.
		 */
		enum e_method get_method() const;

		/**
		 * Get the request target.
		 * @throw	runtime_error	Request target wasn't set yet.
		 * @return	Request target.
		 */
		const std::string &get_request_target() const;

		/**
		 * Get the request query.
		 * @throw	runtime_error	Request query wasn't set yet.
		 * @return	Request query (empty string if wasn't present).
		 */
		const std::string &get_request_query() const;

		/**
		 * Get the value of a header with the \p key.
		 * @throw	range_error	Header field with such a key
		 * 				wasn't parsed.
		 * @param	key	Key of the header.
		 * @return	Value of the requested header.
		 */
		const std::string &get_header_value(const std::string &key) const;

		/**
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
		// All possible information from the start line.
		enum e_method _method;
		bool _method_is_set;		// To check if `_method` is initialized.
		std::string _request_target;
		bool _request_target_is_set;
		std::string _request_query;	// Optional.
		bool _request_query_is_set;

		// Header fields in format "key:OWS value OWS".
		//
		// All requests must contain a "Host" field.
		// Without it, the server should respond with 400.
		//
		// Additionally, POST method must also include
		// either the "Content-Length" or "Transfer-Encoding" field.
		// If both are present, "Transfer-Encoding" takes precedence.
		// Without both, the server should respond with 411.
		std::map<std::string, std::string> _header_fields;

		bool _complete;			// If request is fully parsed.



		/**
		 * Parse method, request target and
		 * optional request query (if present) from \p start_line.
		 * @throw	invalid_argument	The start line stored
		 * 					in \p info is malformed.
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
		 * Set the `_request_target` and `_request_query`
		 * to those from \p start_line.
		 * @throw	invalid_argument	\p start_line
		 * 					contains
		 * 					invalid information.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @param	pos		Where the request target
		 * 				in \p start_line starts.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t set_request_target_and_query(const std::string &start_line,
				size_t pos);

		/**
		 * Parses a request component from \p start_line
		 * beginning at \p pos and ending at \p end.
		 * @warning	If any information is already set in
		 * 		\p component, it will be lost.
		 * @throw	invalid_argument	\p start_line
		 * 					contains
		 * 					invalid information.
		 * @param	component	Component to set.
		 * 				percent-unencoded character.
		 * @param	start_line	The start line of the request
		 * 				with the "\r\n" erased.
		 * @param	pos		Where the request component
		 * 				in \p start_line starts.
		 * @param	end		Where does the request component end.
		 * @return	Processed bytes in \p start_line.
		 */
		size_t set_request_component(std::string &component,
				const std::string &start_line, size_t pos, size_t end);

		/**
		 * Decodes the percent-encoded character stored in
		 * \p start_line at \p pos.
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
};
