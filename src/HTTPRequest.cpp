#include "HTTPRequest.hpp"
#include <cstddef>
#include <string>
#include <stdexcept>
#include <vector>

HTTPRequest::HTTPRequest()
	: _complete(false)
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
	else
	{
		return this->handle_header_field(info.substr(0, ft_pos))
			+ FIELD_TERMINATOR.length();
	}
}

size_t HTTPRequest::handle_start_line(const std::string &start_line)
{
	size_t i;
	const std::string AFTER_METHOD = " /";
	const std::string START_LINE_END = " HTTP/1.1"

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

size_t HTTPRequest::set_request_target_and_query(const std::string &start_line,
		size_t pos)
{
	size_t end;

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
	while (pos < end)
	{
	}
}
