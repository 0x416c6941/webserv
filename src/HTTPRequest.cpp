#include "HTTPRequest.hpp"
#include <cstddef>
#include <string>
#include <stdexcept>
#include <vector>
#include <cctype>

HTTPRequest::HTTPRequest()
	: _method_is_set(false), _complete(false)
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
	if (!(this->_method_is_set) || this->_request_target.empty())
	{
		return this->handle_start_line(info.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
	/*
	else
	{
		return this->handle_header_field(info.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
	 */
	return 0;	// TODO.
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

// It's possible to divide this function for better code structure.
size_t HTTPRequest::set_request_target_and_query(const std::string &start_line,
		size_t pos)
{
	size_t end;
	size_t i = pos;
	// RFC 3986.
	const std::string ALLOWED_UNENCODED_CHARS =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"-_~.";
	const std::string ALLOWED_ENCODED_CHARS =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"-_~."
		"!#$&'()*+,/:;=?@[]";

	// Finding the end of the request target.
	end = start_line.find('?', pos);
	if (end == std::string::npos)
	{
		end = start_line.find(' ', pos);
	}
	if (end == std::string::npos)
	{
		end = start_line.length();
	}
	// _request_target.
	while (i < end)
	{
		if (start_line.at(i) == '%')
		{
			// Percent-encoded character.
			char pec = this->decode_percent_encoded_character(start_line, i);
			if (ALLOWED_ENCODED_CHARS.find(start_line.at(i)) == std::string::npos)
			{
				throw std::invalid_argument("HTTPRequest::set_request_target_and_query(): Start line contains illegal encoded characters.");
			}
			_request_target.push_back(pec);
		}
		else if (ALLOWED_UNENCODED_CHARS.find(start_line.at(i)) != std::string::npos)
		{
			_request_target.push_back(start_line.at(i++));
		}
		else
		{
			throw std::invalid_argument("HTTPRequest::set_request_target_and_query(): Start line contains illegal unencoded characters.");
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
				throw std::range_error("HTTPRequest::decode_percent_encoded_character(): Expected literal after % sign.");
			}
			else if (std::isdigit(start_line.at(pos)))
			{
				ret = ret * BASE + (start_line.at(pos) - '0');
			}
			else if (std::isupper(start_line.at(pos))
				|| std::islower(start_line.at(pos)))
			{
				ret = ret * BASE + (std::toupper(start_line.at(pos)) - 'A' + 10);
			}
			else
			{
				throw std::invalid_argument("HTTPRequest::decode_percent_encoded_character(): Start line contains invalid information.");
			}
			pos++;
		}
	}
	return ret;
}
