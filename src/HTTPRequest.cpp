#include "HTTPRequest.hpp"
#include <cstddef>
#include <string>
#include <stdexcept>
#include <netinet/in.h>
#include <cstring>
#include "Webserv.hpp"		// utils.
#include <cctype>
#include <inttypes.h>
#include <cstdlib>
#include <iostream>		// Debug.
#include <iomanip>		// Debug.

HTTPRequest::HTTPRequest()
	:	_server_address_is_set(false),
		_client_address_is_set(false),
		_method_is_set(false),
		_request_path_is_set(false),
		_request_query_is_set(false),
		_request_target_is_set(false),
		_header_complete(false),
		_body_complete(false)
{
	(void) memset(&_server_address, 0, sizeof(struct sockaddr_in));
	(void) memset(&_client_address, 0, sizeof(struct sockaddr_in));
}

HTTPRequest::~HTTPRequest()
{
}

void HTTPRequest::reset()
{
	(void) memset(&_server_address, 0, sizeof(struct sockaddr_in));
	_server_address_is_set = false;
	(void) memset(&_client_address, 0, sizeof(struct sockaddr_in));
	_client_address_is_set = false;
	_method_is_set = false;
	_request_path_original.clear();
	_request_path_decoded.clear();
	_request_path_is_set = false;
	_request_query_original.clear();
	_request_query_decoded.clear();
	_request_query_is_set = false;
	_request_target.clear();
	_request_target_is_set = false;
	_header_fields.clear();
	_header_complete = false;
	_body.clear();
	_body_complete = false;
}

HTTPRequest::method_not_allowed::method_not_allowed(const char * msg)
	: _MSG(msg)
{
}

HTTPRequest::method_not_allowed::method_not_allowed(const std::string &msg)
	: _MSG(msg)
{
}

HTTPRequest::method_not_allowed::~method_not_allowed() throw()
{
}

const char * HTTPRequest::method_not_allowed::what() const throw()
{
	return _MSG.c_str();
}

HTTPRequest::http_ver_unsupported::http_ver_unsupported(const char * msg)
	: _MSG(msg)
{
}

HTTPRequest::http_ver_unsupported::http_ver_unsupported(const std::string &msg)
	: _MSG(msg)
{
}

HTTPRequest::http_ver_unsupported::~http_ver_unsupported() throw()
{
}

const char * HTTPRequest::http_ver_unsupported::what() const throw()
{
	return _MSG.c_str();
}

HTTPRequest::non_ascii_request::non_ascii_request(const char * msg)
	: _MSG(msg)
{
}

HTTPRequest::non_ascii_request::non_ascii_request(const std::string &msg)
	: _MSG(msg)
{
}

HTTPRequest::non_ascii_request::~non_ascii_request() throw()
{
}

const char * HTTPRequest::non_ascii_request::what() const throw()
{
	return _MSG.c_str();
}

void HTTPRequest::set_server_address(const struct sockaddr_in &server_address)
{
	(void) memcpy(&_server_address, &server_address,
		sizeof(struct sockaddr_in));
	this->_server_address_is_set = true;
}

const struct sockaddr_in &HTTPRequest::get_server_address() const
{
	if (!_server_address_is_set)
	{
		throw std::runtime_error(std::string("HTTPRequest::get_server_address(): ")
				+ "_server_address is not set yet.");
	}
	return _server_address;
}

void HTTPRequest::set_client_address(const struct sockaddr_in &client_address)
{
	(void) memcpy(&_client_address, &client_address,
		sizeof(struct sockaddr_in));
	this->_client_address_is_set = true;
}

const struct sockaddr_in &HTTPRequest::get_client_address() const
{
	if (!_client_address_is_set)
	{
		throw std::runtime_error(std::string("HTTPRequest::get_client_address(): ")
				+ "_client_address is not set yet.");
	}
	return _client_address;
}

size_t HTTPRequest::process_header_line(const std::string &header_line)
{
	size_t ft_pos;	// Field terminator position.
	const std::string FIELD_TERMINATOR = "\r\n";

	if (this->_header_complete)
	{
		throw std::range_error("HTTPRequest::process_header_line(): Request has already been fully parsed.");
	}
	ft_pos = header_line.find(FIELD_TERMINATOR);
	if (ft_pos == std::string::npos)
	{
		throw std::invalid_argument("HTTPRequest::process_header_line(): Some line isn't properly terminated.");
	}
	else if (ft_pos == 0)
	{
		if (!(this->_method_is_set))
		{
			throw std::invalid_argument("HTTPRequest::process_header_line(): Found \"r\"n immediately after the request beginning.");
		}
		else if (this->_header_fields.find("Host")
			== this->_header_fields.end())
		{
			throw std::runtime_error("HTTPRequest::process_header_line(): \"Host\" header field isn't present.");
		}
		this->_header_complete = true;
		return FIELD_TERMINATOR.length();
	}
	if (!(this->_method_is_set) || !(this->_request_target_is_set))
	{
		return this->handle_start_line(header_line.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
	else
	{
		return this->handle_header_field(header_line.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
}

enum HTTPRequest::e_method HTTPRequest::get_method() const
{
	if (!(this->_method_is_set))
	{
		throw std::runtime_error("HTTPRequest::get_method(): Request method wasn't set yet.");
	}
	return this->_method;
}

const std::string &HTTPRequest::get_request_path_original() const
{
	if (!(this->_request_path_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_path_original(): ")
				+ "Request path wasn't set yet.");
	}
	return this->_request_path_original;
}

const std::string &HTTPRequest::get_request_path_decoded() const
{
	if (!(this->_request_path_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_path_decoded(): ")
				+ "Request path wasn't set yet.");
	}
	return this->_request_path_decoded;
}

std::string HTTPRequest::get_request_path_decoded_strip_location_path(
		const std::string &loc_path) const {
	std::string ret;

	if (!(this->_request_path_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_path_decoded_strip_location_path(): ")
				+ "Request path wasn't set yet.");
	}
	else if (loc_path.length() <= 0 || loc_path.at(0) != '/'
		|| loc_path.at(loc_path.length() - 1) != '/')
	{
		throw std::invalid_argument(std::string("HTTPRequest::get_request_path_decoded_strip_location_path(): ")
				+ "Provided location path doesn't start or end with '/'.");
	}
	if (this->_request_path_decoded.compare(0,
				loc_path.length(), loc_path) != 0)
	{
		throw std::domain_error(std::string("HTTPRequest::get_request_path_decoded_strip_location_path(): ")
				+ "Provided location path isn't contained in the request path.");
	}
	ret = this->_request_path_decoded;
	ret.erase(0, loc_path.length());
	return ret;
}

const std::string &HTTPRequest::get_request_query_original() const
{
	if (!(this->_request_query_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_query_original(): ")
				+ "Request query wasn't set yet.");
	}
	return this->_request_query_original;
}

const std::string &HTTPRequest::get_request_query_decoded() const
{
	if (!(this->_request_query_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_query_decoded(): ")
				+ "Request query wasn't set yet.");
	}
	return this->_request_query_decoded;
}

const std::string &HTTPRequest::get_request_target() const
{
	if (!(this->_request_target_is_set))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_request_target(): ")
				+ "Request target wasn't set yet.");
	}
	return this->_request_target;
}

const std::string &HTTPRequest::get_header_value(const std::string &key) const
{
	if (this->_header_fields.find(key) == this->_header_fields.end())
	{
		throw std::range_error("HTTPRequest::get_header_value(): Header with the provided key wasn't set yet.");
	}
	return this->_header_fields.at(key);
}

const std::map<std::string, std::string> &HTTPRequest::get_header_fields() const
{
	if (!(this->_header_complete))
	{
		throw std::runtime_error(std::string("HTTPRequest::get_header_fields(): ")
				+ "Request's header isn't fully parsed yet.");
	}
	return this->_header_fields;
}

size_t HTTPRequest::process_body_part(const std::string &buffer)
{
	if (this->_body_complete)
	{
		throw std::range_error("HTTPRequest::process_body_part(): Body has already been fully parsed.");
	}
	else if (this->_header_fields.find("Content-Length")
		!= this->_header_fields.end())
	{
		return this->process_body_part_cl(buffer);
	}
	else if (this->_header_fields.find("Transfer-Encoding")
		!= this->_header_fields.end())
	{
		return this->process_body_part_te(buffer);
	}
	throw std::domain_error("HTTPRequest::process_body_part(): Have neither Content-Length nor Transfer-Encoding headers.");
}

const std::string &HTTPRequest::get_body() const
{
	return this->_body;
}

bool HTTPRequest::is_header_complete() const
{
	return this->_header_complete;
}

bool HTTPRequest::is_body_complete() const
{
	if (!(this->_method_is_set)
		|| (this->_method != POST && this->_method != PUT))
	{
		throw std::domain_error(std::string("HTTPRequest::is_body_complete(): ")
				+ "Request's method isn't \"POST\" neither \"PUT\".");
	}
	return this->_body_complete;
}

bool HTTPRequest::is_complete() const
{
	if ((this->_method_is_set
		&& (this->_method == GET || this->_method == DELETE))
		&& this->_header_complete)
	{
		return true;
	}
	else if ((this->_method_is_set
		&& (this->_method == POST || this->_method == PUT))
		&& (this->_header_complete && this->_body_complete))
	{
		return true;
	}
	return false;
}

size_t HTTPRequest::handle_start_line(const std::string &start_line)
{
	size_t i;
	const std::string	START_LINE_END_HTTP_PREFIX = " HTTP/",
				HTTP_VERSION_1_1 = "1.1";

	i = this->set_method(start_line);
	// ' ' after the request method.
	if (start_line.length() <= i || start_line.at(i) != ' ')
	{
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	i++;
	// Request path, query and target.
	if (start_line.length() <= i)
	{
		// Got e.g. "GET " as a start line.
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	i += this->set_request_path_query_and_target(start_line, i);
	// " HTTP/" after request target.
	if (start_line.length() <= i + START_LINE_END_HTTP_PREFIX.length()
		|| start_line.compare(i, START_LINE_END_HTTP_PREFIX.length(),
			START_LINE_END_HTTP_PREFIX) != 0)
	{
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	i += START_LINE_END_HTTP_PREFIX.length();
	// Checking HTTP version (we support only HTTP/1.1).
	if (start_line.length() != i + HTTP_VERSION_1_1.length()
		|| start_line.compare(i, HTTP_VERSION_1_1.length(),
			HTTP_VERSION_1_1) != 0)
	{
		throw http_ver_unsupported(std::string("HTTPRequest::handle_start_line(): ")
				+ "Request's HTTP version is unsupported.");
	}
	return i + HTTP_VERSION_1_1.length();
}

size_t HTTPRequest::set_method(const std::string &start_line)
{
	const std::string	GET_STR = "GET",
				POST_STR = "POST",
				DELETE_STR = "DELETE",
				PUT_STR = "PUT";

	if (start_line.compare(0, GET_STR.length(), GET_STR) == 0)
	{
		_method = GET;
		_method_is_set = true;
		return GET_STR.length();
	}
	else if (start_line.compare(0, POST_STR.length(), POST_STR) == 0)
	{
		_method = POST;
		_method_is_set = true;
		return POST_STR.length();
	}
	else if (start_line.compare(0, DELETE_STR.length(), DELETE_STR) == 0)
	{
		_method = DELETE;
		_method_is_set = true;
		return DELETE_STR.length();
	}
	else if (start_line.compare(0, PUT_STR.length(), PUT_STR) == 0)
	{
		_method = PUT;
		_method_is_set = true;
		return PUT_STR.length();
	}
	throw method_not_allowed(std::string("HTTPRequest::set_method(): ")
			+ "Request method isn't supported.");
}

// Request query may be empty,
// e.g. this start line: "GET /? HTTP/1.1\r\n" is perfectly valid.
size_t HTTPRequest::set_request_path_query_and_target(const std::string &start_line,
		size_t pos)
{
	size_t ret = 0;
	size_t end;

	// Finding the end of the request path.
	end = start_line.find('?', pos);
	if (end == std::string::npos)
	{
		end = start_line.find(' ', pos);
		if (end == std::string::npos)
		{
			end = start_line.length();
		}
	}
	// `_request_path*` and appending encoded version to `_request_target`.
	ret += this->set_request_component(
			this->_request_path_original, this->_request_path_decoded,
			start_line, pos, end);
	this->_request_path_is_set = true;
	this->_request_target = this->_request_path_original;
	pos += ret;
	// Checking if the encoded request path is in origin form.
	if (this->_request_path_original.length() <= 0
		|| this->_request_path_original.at(0) != '/')
	{
		throw std::invalid_argument(std::string("HTTPRequest::set_request_path_query_and_target(): ")
				+ "Only origin form is supported as an encoded request path.");
	}
	else if (!(this->request_path_decoded_no_double_slash_anywhere(
				this->_request_path_decoded)))
	{
		// Decoded request path contains two or more consequent slashes,
		// which shouldn't be the case.
		throw std::invalid_argument(std::string("HTTPRequest::set_request_path_query_and_target(): ")
				+ "Decoded request path contains two or more consequent slashes.");
	}
	else if (start_line.length() <= pos || start_line.at(pos) != '?')
	{
		// Optional query isn't present.
		this->_request_target_is_set = true;
		return ret;
	}
	// `_request_query*` and appending it to `_request_target`.
	if (start_line.length() <= ++pos)
	{
		throw std::invalid_argument(std::string("HTTPRequest::set_request_path_query_and_target(): ")
				+ "Start line unexpectedly ends after ? sign.");
	}
	_request_target.push_back('?');
	ret++;				// '?' sign.
	// Finding the end of the request query.
	end = start_line.find(' ', pos);
	if (end == std::string::npos)
	{
		end = start_line.length();
	}
	// `_request_query*` and appending encoded version to `_request_target`.
	ret += this->set_request_component(
			this->_request_query_original, this->_request_query_decoded,
			start_line, pos, end);
	this->_request_query_is_set = true;
	this->_request_target += this->_request_query_original;
	this->_request_target_is_set = true;
	pos += ret;
	return ret;
}

size_t HTTPRequest::set_request_component(
		std::string &comp_original, std::string &comp_decoded,
		const std::string &start_line, const size_t POS, const size_t END)
{
	size_t i = POS;
	// RFC 3986 in case of request path and request query.
	const std::string ALLOWED_UNENCODED_CHARS =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"-_~."
		"!$&'()*+,/:;=@";
	const std::string ALLOWED_CHARS_ONLY_ENCODED =
		"#?[] %";

	if (POS > END)
	{
		throw std::invalid_argument(std::string("HTTPRequest::set_request_component(): ")
				+ "POS: " + to_string(POS) +
				+ " is bigger than END: " + to_string(END) + '.');
	}
	// If any information is already stored in `comp_original`
	// or `comp_decoded`, drop it.
	comp_original.clear();
	comp_decoded.clear();
	while (i < END)
	{
		if (start_line.at(i) == '%')
		{
			// Percent-encoded character.
			char pec = this->decode_percent_encoded_character(start_line, i);
			if (ALLOWED_UNENCODED_CHARS.find(pec) == std::string::npos
				&& ALLOWED_CHARS_ONLY_ENCODED.find(pec) == std::string::npos)
			{
				throw std::invalid_argument(std::string("HTTPRequest::set_request_component(): ")
						+ "Start line contains illegal encoded characters.");
			}
			comp_decoded.push_back(pec);
		}
		else if (ALLOWED_UNENCODED_CHARS.find(start_line.at(i)) != std::string::npos)
		{
			comp_decoded.push_back(start_line.at(i++));
		}
		else
		{
			throw std::invalid_argument(std::string("HTTPRequest::set_request_component(): ")
					+ "Start line contains illegal encoded characters.");
		}
	}
	comp_original = start_line.substr(POS, END - POS);
	return i - POS;
}

bool HTTPRequest::request_path_decoded_no_double_slash_anywhere(
		const std::string &request_path_decoded) const {
	bool previous_char_was_slash = false;

	for (size_t i = 0; i < request_path_decoded.length(); i++) {
		if (request_path_decoded.at(i) == '/') {
			if (previous_char_was_slash) {
				return false;
			}
			previous_char_was_slash = true;
		}
		else {
			previous_char_was_slash = false;
		}
	}
	return true;
}

// Percent encoding is only a thing in start line.
// Or, well, if it can be everywhere, we only support a subset of RFC.
char HTTPRequest::decode_percent_encoded_character(const std::string &start_line,
		size_t &pos)
{
	char ret = 0;
	const size_t LITERALS_AFTER_PERCENT = 2;	// E.g. "%20".
	const int BASE = 0x10;				// 16, since working with HEX.

	++pos;	// Skip '%'.
	// We write a PoC.
	// Complex webservers would find that e.g. "%D0%BC" is unicode
	// and interpret this sequence as one character, however in our case
	// such support of sequence of bytes is redundant.
	for (size_t i = 0; i < LITERALS_AFTER_PERCENT; i++)
	{
		if (!(pos < start_line.length()))
		{
			throw std::invalid_argument(std::string("HTTPRequest::decode_percent_encoded_character(): ")
					+ "Expected some literal after % sign.");
		}
		else if (std::isdigit(start_line.at(pos)))
		{
			if (static_cast<char>(ret * BASE + (start_line.at(pos) - '0'))
				< ret)
			{
				throw non_ascii_request(std::string("HTTPRequest::decode_percent_encoded_character(): ")
						+ "Only ASCII characters are supported as percent-encoded characters.");
			}
			ret = ret * BASE + (start_line.at(pos) - '0');
		}
		else if (std::toupper(start_line.at(pos)) >= 'A'
			&& std::toupper(start_line.at(pos)) <= 'F')
		{
			if (static_cast<char>(ret * BASE + (std::toupper(start_line.at(pos)) - 'A' + 10))
				< ret)
			{
				throw non_ascii_request(std::string("HTTPRequest::decode_percent_encoded_character(): ")
						+ "Only ASCII characters are supported as percent-encoded characters.");
			}
			ret = ret * BASE + (std::toupper(start_line.at(pos)) - 'A' + 10);
		}
		else
		{
			throw std::invalid_argument("HTTPRequest::decode_percent_encoded_character(): Some literal after % sign is invalid.");
		}
		pos++;
	}
	return ret;
}

size_t HTTPRequest::handle_header_field(const std::string &header_field)
{
	const std::string DELIM = ":";
	size_t delim_pos, value_begin_pos, value_end_pos;
	std::string key;
	std::string value;

	// Key.
	delim_pos = header_field.find(DELIM);
	if (delim_pos == 0)
	{
		throw std::invalid_argument("HTTPRequest::handle_header_field(): Header field's key is empty.");
	}
	else if (delim_pos == std::string::npos)
	{
		throw std::invalid_argument("HTTPRequest::handle_header_field(): Header field is malformed.");
	}
	for (size_t i = 0; i < delim_pos; i++)
	{
		if (!std::isgraph(header_field.at(i)))
		{
			throw std::invalid_argument("HTTPRequest::handle_header_field(): Header field's key must consist only of printable non-whitespace characters.");
		}
		key.push_back(header_field.at(i));
	}
	if (this->_header_fields.find(key) != this->_header_fields.end())
	{
		throw std::runtime_error("HTTPRequest::handle_header_field(): Header field is duplicated.");
	}
	// Value.
	value_begin_pos = delim_pos + DELIM.length();
	// Skipping the optional whitespaces after the delimiter.
	while (value_begin_pos < header_field.length()
		&& std::isspace(header_field.at(value_begin_pos)))
	{
		if (header_field.at(value_begin_pos) != ' '
			&& header_field.at(value_begin_pos) != '\t')
		{
			throw std::invalid_argument("HTTPRequest::handle_header_field: Header field contains illegal whitespace.");
		}
		value_begin_pos++;
	}
	value_end_pos = header_field.length() - 1;	// No overflow will happen at this point.
	// Skipping the optional whitespaces after the header's value.
	while (value_end_pos != 0
		&& std::isspace(header_field.at(value_end_pos)))
	{
		if (header_field.at(value_end_pos) != ' '
			&& header_field.at(value_end_pos) != '\t')
		{
			throw std::invalid_argument("HTTPRequest::handle_header_field: Header field contains illegal whitespace.");
		}
		value_end_pos--;
	}
	// It's fine even if value is empty.
	for (size_t i = value_begin_pos; i <= value_end_pos; i++)
	{
		if (!std::isprint(header_field.at(i)))
		{
			throw std::invalid_argument("HTTPRequest::handle_header_field(): Header field's value must consist only of printable characters.");
		}
		value.push_back(header_field.at(i));
	}
	(this->_header_fields)[key] = value;
	// At this point, all bytes in `_header_field` were processed.
	return header_field.length();
}

size_t HTTPRequest::process_body_part_cl(const std::string &buffer)
{
	const char * const CL_STR = this->_header_fields.at("Content-Length").c_str();
	char * conv_err_check;	// To check for errors when using strtoumax().
	const int STRTOUMAX_BASE = 10;
	unsigned cl_bytes;		// Number in "Content-Length" header.
	unsigned bytes_to_append;

	if (this->_body_complete)
	{
		throw std::range_error("HTTPRequest::process_body_part_cl(): Body was already processed.");
	}
	errno = 0;	// strtoumax() doesn't modify errno on success.
	cl_bytes = strtoumax(CL_STR, &conv_err_check, STRTOUMAX_BASE);
	// Checking for strtoumax() errors.
	if (*CL_STR == '\0' || *conv_err_check != '\0'
		|| errno == ERANGE)
	{
		throw std::runtime_error("HTTPRequest::process_body_part_cl(): \"Content-Length\" header doesn't contain a valid number.");
	}
	bytes_to_append = cl_bytes - this->_body.length();	// Underflow should never happen.
	this->_body.append(buffer, 0, bytes_to_append);
	if (this->_body.length() == cl_bytes)
	{
		this->_body_complete = true;
		return bytes_to_append;		// We've appended only this part of buffer.
	}
	return buffer.length();	// We've appended the whole buffer.
}

size_t HTTPRequest::process_body_part_te(const std::string &buffer)
{
	const char * const BUFFER_C_STR = buffer.c_str();
	char * conv_err_check;	// To check for errors when using strtoul().
	const int STRTOUL_BASE = 16;
	unsigned long bytes_to_append;
	size_t pos;
	const std::string TERMINATOR = "\r\n";

	if (this->_body_complete)
	{
		throw std::range_error("HTTPRequest::process_body_part_te(): Body was already processed.");
	}
	// Weird case, but let's still handle it.
	else if (buffer.length() == 0)
	{
		throw std::invalid_argument("HTTPRequest::process_body_part_te(): Buffer is empty.");
	}
	errno = 0;	// strtoul() doesn't modify errno on success.
	bytes_to_append = std::strtoul(BUFFER_C_STR, &conv_err_check, STRTOUL_BASE);
	// strtoul() didn't report any error and buffer contains
	// only a number of bytes in the chunk.
	if (*conv_err_check == '\0' && errno == 0)
	{
		throw std::invalid_argument("HTTPRequest::process_body_part_te(): Buffer doesn't contain a complete chunk.");
	}
	else if (*conv_err_check != '\r' || errno == ERANGE)
	{
		throw std::runtime_error("HTTPRequest::process_body_part_te(): Body part is borked.");
	}
	pos = static_cast<size_t> (conv_err_check - BUFFER_C_STR);
	// Checking for terminator after number of bytes in the chunk.
	if (buffer.length() < pos + TERMINATOR.length())
	{
		throw std::invalid_argument("HTTPRequest::process_body_part_te(): Buffer doesn't contain a complete chunk.");
	}
	else if (buffer.compare(pos, TERMINATOR.length(), TERMINATOR) != 0)
	{
		throw std::runtime_error("HTTPRequest::process_body_part_te(): Body part is borked.");
	}
	pos += TERMINATOR.length();
	if (buffer.length() < pos + bytes_to_append + TERMINATOR.length())
	{
		throw std::invalid_argument("HTTPRequest::process_body_part_te(): Buffer doesn't contain a complete chunk.");
	}
	// Checking for the chunk's enclosing terminator.
	else if (buffer.compare(pos + bytes_to_append, TERMINATOR.length(), TERMINATOR) != 0)
	{
		throw std::runtime_error("HTTPRequest::process_body_part_te(): Body part is borked.");
	}
	this->_body.append(buffer, pos, bytes_to_append);
	if (bytes_to_append == 0)
	{
		this->_body_complete = true;
	}
	return pos + bytes_to_append + TERMINATOR.length();
}


void HTTPRequest::printDebug() const {
	std::cout << "\n===== HTTP Request Debug Info =====" << std::endl;

	// Method
	if (_method_is_set) {
		std::string method_str;
		switch (_method) {
			case GET:    method_str = "GET"; break;
			case POST:   method_str = "POST"; break;
			case DELETE: method_str = "DELETE"; break;
			case PUT:    method_str = "PUT"; break;
			default:     method_str = "UNKNOWN"; break;
		}
		std::cout << "Method:          " << method_str << std::endl;
	} else {
		std::cout << "Method:          [NOT SET]" << std::endl;
	}

	// Path.
	if (_request_path_is_set)
	{
		std::cout << "Path original:   " << _request_path_original << std::endl;
		std::cout << "Path decoded:    " << _request_path_decoded << std::endl;
	}
	else
	{
		std::cout << "Path:            [NOT SET]" << std::endl;
	}
	// Query.
	if (_request_query_is_set)
	{
		std::cout << "Query original:  " << _request_query_original << std::endl;
		std::cout << "Query decoded:   " << _request_query_decoded << std::endl;
	}
	else
	{
		std::cout << "Query:           [NOT SET]" << std::endl;
	}
	// Target.
	if (_request_target_is_set)
	{
		std::cout << "Target:          " << _request_target << std::endl;
	}
	else
	{
		std::cout << "Target:          [NOT SET]" << std::endl;
	}
	// Headers.
	std::cout << "Header Fields:" << std::endl;
	if (_header_fields.empty())
	{
		std::cout << "  [NONE]" << std::endl;
	}
	else
	{
		for (std::map<std::string, std::string>::const_iterator it = _header_fields.begin();
			it != _header_fields.end(); ++it)
		{
			std::cout << "  " << std::setw(16) << std::left << it->first
				  << ": " << it->second << std::endl;
		}
	}
	// Completion flag.
	std::cout << "Request complete: " << (this->is_complete() ? "Yes" : "No") << std::endl;
	if (_body_complete)
	{
		std::cout << "Body:" << std::endl;
		std::cout << "================" << std::endl;
		std::cout << _body << std::endl;
		std::cout << "================" << std::endl;
	}
	std::cout << "====================================\n" << std::endl;
}
