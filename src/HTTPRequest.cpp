#include "HTTPRequest.hpp"
#include <cstddef>
#include <string>
#include <stdexcept>
#include <vector>
#include <cctype>

HTTPRequest::HTTPRequest()
	:	_method_is_set(false),
		_request_target_is_set(false),
		_request_query_is_set(false),
		_complete(false)
{
}

HTTPRequest::~HTTPRequest()
{
}

size_t HTTPRequest::process_info(const std::string &info)
{
	size_t ft_pos;	// Field terminator position.
	const std::string FIELD_TERMINATOR = "\r\n";

	if (this->_complete)
	{
		throw std::range_error("HTTPRequest::process_info(): Request has already been fully parsed.");
	}
	ft_pos = info.find(FIELD_TERMINATOR);
	if (ft_pos == std::string::npos)
	{
		throw std::invalid_argument("HTTPRequest::process_info(): Some line isn't properly terminated.");
	}
	else if (ft_pos == 0)
	{
		if (!(this->_method_is_set))
		{
			throw std::invalid_argument("HTTPRequest::process_info(): Found \"r\"n immediately after the request beginning.");
		}
		this->_complete = true;
		return FIELD_TERMINATOR.length();
	}
	if (!(this->_method_is_set) || !(this->_request_target_is_set))
	{
		return this->handle_start_line(info.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
	else
	{
		return this->handle_header_field(info.substr(0, ft_pos))
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

const std::string &HTTPRequest::get_request_target() const
{
	if (!(this->_request_target_is_set))
	{
		throw std::runtime_error("HTTPRequest::get_request_target(): Request target wasn't set yet.");
	}
	return this->_request_target;
}

const std::string &HTTPRequest::get_request_query() const
{
	if (!(this->_request_query_is_set))
	{
		throw std::runtime_error("HTTPRequest::get_request_query(): Request query wasn't set yet.");
	}
	return this->_request_query;
}

const std::string &HTTPRequest::get_header_value(const std::string &key) const
{
	if (this->_header_fields.find(key) == this->_header_fields.end())
	{
		throw std::range_error("HTTPRequest::get_header_value(): Header with the provided key wasn't set yet.");
	}
	return this->_header_fields.at(key);
}

bool HTTPRequest::is_complete() const
{
	return this->_complete;
}

size_t HTTPRequest::handle_start_line(const std::string &start_line)
{
	size_t i;
	const std::string AFTER_METHOD = " /";
	const std::string START_LINE_END = " HTTP/1.1";

	i = this->set_method(start_line);
	// " /" after the request method.
	if (start_line.length() <= i
		|| start_line.compare(i, AFTER_METHOD.length(), AFTER_METHOD) != 0)
	{
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	i += AFTER_METHOD.length();
	// Request target and query.
	if (start_line.length() <= i)
	{
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	i += this->set_request_target_and_query(start_line, i);
	// " HTTP/1.1" after request target and query.
	if (start_line.length() != i + START_LINE_END.length()
		|| start_line.compare(i, START_LINE_END.length(), START_LINE_END) != 0)
	{
		throw std::invalid_argument("HTTPRequest::handle_start_line(): Start line is malformed.");
	}
	return i + START_LINE_END.length();
}

size_t HTTPRequest::set_method(const std::string &start_line)
{
	const std::string GET_STR = "GET", POST_STR = "POST", DELETE_STR = "DELETE";

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
	throw std::invalid_argument("HTTPRequest::set_method(): Request method isn't supported.");
}

// Both request target and query may be empty,
// e.g. this start line: "GET /? HTTP/1.1\r\n" is perfectly valid.
size_t HTTPRequest::set_request_target_and_query(const std::string &start_line,
		size_t pos)
{
	size_t ret = 0;
	size_t end;

	// Finding the end of the request target.
	end = start_line.find('?', pos);
	if (end == std::string::npos)
	{
		end = start_line.find(' ', pos);
		if (end == std::string::npos)
		{
			end = start_line.length();
		}
	}
	// _request_target.
	ret += this->set_request_component(this->_request_target,
			start_line, pos, end);
	this->_request_target_is_set = true;
	pos += ret;
	if (start_line.length() <= pos || start_line.at(pos) != '?')
	{
		// Optional query isn't present.
		return ret;
	}
	// _request_query.
	if (start_line.length() <= ++pos)
	{
		throw std::invalid_argument("HTTPRequest::set_request_target_and_query(): Start line unexpectedly ends after ? sign.");
	}
	ret++;	// Skipped the '?' sign.
	// Finding the end of the request query.
	// Request query may also be empty, as well request target,
	// e.g. this start line: "GET /? HTTP/1.1\r\n" is valid.
	end = start_line.find(' ', pos);
	if (end == std::string::npos)
	{
		end = start_line.length();
	}
	ret += this->set_request_component(this->_request_query,
			start_line, pos, end);
	this->_request_query_is_set = true;
	pos += ret;
	return ret;
}

size_t HTTPRequest::set_request_component(std::string & component,
		const std::string &start_line, size_t pos, size_t end)
{
	size_t i = pos;
	// RFC 3986 in case of path in request target and request query.
	const std::string ALLOWED_UNENCODED_CHARS =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"-_~."
		"!$&'()*+,/:;=@";
	const std::string ALLOWED_CHARS_ONLY_ENCODED =
		"#?[] %";

	// If any information is already stored in `component`, drop it.
	component.clear();
	while (i < end)
	{
		if (start_line.at(i) == '%')
		{
			// Percent-encoded character.
			char pec = this->decode_percent_encoded_character(start_line, i);
			if (ALLOWED_UNENCODED_CHARS.find(pec) == std::string::npos
				&& ALLOWED_CHARS_ONLY_ENCODED.find(pec) == std::string::npos)
			{
				throw std::invalid_argument("HTTPRequest::set_request_component(): Start line contains illegal encoded characters.");
			}
			component.push_back(pec);
		}
		else if (ALLOWED_UNENCODED_CHARS.find(start_line.at(i)) != std::string::npos)
		{
			component.push_back(start_line.at(i++));
		}
		else
		{
			throw std::invalid_argument("HTTPRequest::set_request_component(): Start line contains illegal unencoded characters.");
		}
	}
	return i - pos;
}

// Percent encoding is only a thing in start line.
char HTTPRequest::decode_percent_encoded_character(const std::string &start_line,
		size_t &pos)
{
	char ret = 0;
	const size_t LITERALS_AFTER_PERCENT = 2;	// E.g. "%20".
	const int BASE = 0x10;				// 16, since working with HEX.

	while (pos < start_line.length() && start_line.at(pos) == '%')
	{
		pos++;	// Skip '%'.
		for (size_t i = 0; i < LITERALS_AFTER_PERCENT; i++)
		{
			if (!(pos < start_line.length()))
			{
				throw std::invalid_argument("HTTPRequest::decode_percent_encoded_character(): Expected some literal after % sign.");
			}
			else if (std::isdigit(start_line.at(pos)))
			{
				if (static_cast<char>(ret * BASE + (start_line.at(pos) - '0'))
					< ret)
				{
					throw std::range_error("HTTPRequest::decode_percent_encoded_character(): Only ASCII characters are supported as percent-encoded characters.");
				}
				ret = ret * BASE + (start_line.at(pos) - '0');
			}
			else if (std::toupper(start_line.at(pos)) >= 'A'
				&& std::toupper(start_line.at(pos)) <= 'F')
			{
				if (static_cast<char>(ret * BASE + (std::toupper(start_line.at(pos)) - 'A' + 10))
					< ret)
				{
					throw std::range_error("HTTPRequest::decode_percent_encoded_character(): Only ASCII characters are supported as percent-encoded characters.");
				}
				ret = ret * BASE + (std::toupper(start_line.at(pos)) - 'A' + 10);
			}
			else
			{
				throw std::invalid_argument("HTTPRequest::decode_percent_encoded_character(): Some literal after % sign is invalid.");
			}
			pos++;
		}
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
		if (!std::isgraph(header_field.at(i)))
		{
			throw std::invalid_argument("HTTPRequest::handle_header_field(): Header field's value must consist only of printable non-whitespace characters.");
		}
		value.push_back(header_field.at(i));
	}
	(this->_header_fields)[key] = value;
	// At this point, all bytes in `_header_field` were processed.
	return header_field.length();
}
