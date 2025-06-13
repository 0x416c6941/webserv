#pragma once

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

		enum e_method
		{
			GET,
			POST,
			DELETE
		};

		/**
		 * Processes line in \p info
		 * until the field terminator ("\r\n") is met:
		 * sets the request method (as well as `_request_target` and `_query` fields),
		 * appends the `_header_fields` with the new field,
		 * or marks the request as complete (fully parsed).
		 * @brief	Process info stored in \p info.
		 * @throw	invalid_argument	\p info isn't terminated
		 * 					or contains invalid information.
		 * @throw	runtime_error		Header fields started
		 * 					before method was set,
		 * 					or request begins with "\r\n".
		 * @throw	range_error		Request was fully parsed,
		 * 					yet new information arrived.
		 * @param	info	Line with information to process.
		 * @return	Processed bytes in \p info.
		 */
		size_t process_info(const std::string &info);

		/**
		 * Get method of the request.
		 * @throw	runtime_error	Method wasn't set yet.
		 * @return	Method of the request.
		 */
		enum e_method get_method() const;

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

	private:
		enum e_method _method;
		bool _method_set;		// To check if `_method` is initialized.
		std::string _request_target;
		std::string _query;		// Optional.

		// All methods must contain a "Host" field.
		// Without it, the server should respond with 400.
		//
		// Additionally, POST must also include
		// either the "Content-Length" or "Transfer-Encoding"
		// field.
		// Without it, the server should respond with 411.
		std::map<std::string, std::string> _header_fields;

		bool _complete;			// If request is fully parsed.

		// It doesn't make sense for `HTTPRequest` to be CopyConstructible
		// or have an ::operator =() available.
		HTTPRequest(const HTTPRequest &src);
		HTTPRequest &operator = (const HTTPRequest &src);

		//bool set_method(const std::string &info);
};
