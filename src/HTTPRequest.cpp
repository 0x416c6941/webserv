#include "HTTPRequest.hpp"
#include <cstddef>
#include <string>
#include <stdexcept>

HTTPRequest::HTTPRequest()
	: _complete(false)
{
}

HTTPRequest::~HTTPRequest()
{
}

size_t HTTPRequest::process_info(const std::string &info)
{
	size_t nl_ft_pos;		// Position of '\n' in "\r\n" terminator.
	const size_t BEGINNING = 1;	// To check for "\r\n" terminator.

	if (this->_complete)
	{
		throw std::range_error("HTTPRequest::process_info(): Request has already been fully parsed.");
	}
	nl_ft_pos = info.find('\n');
	if (nl_ft_pos == std::string::npos
		|| nl_ft_pos == 0 || info.at(nl_ft_pos - 1) != '\r')
	{
		throw std::invalid_argument("HTTPRequest::process_info(): Some line isn't properly terminated.");
	}
	else if (nl_ft_pos == BEGINNING && !(this->_method_set))
	{
		throw std::runtime_error("HTTPRequest::process_info(): Found \"r\"n immediately after the request beginning.");
	}
	return 0;
}
