#include "../include/Webserv.hpp"

/**
* @brief Trims whitespace from both ends of the input string.
* @param str Input string.
* @return Trimmed result.
* @note This method could be made static.
*/
std::string 	trim(const std::string& str) {
	std::size_t start = 0;
	while (start < str.size() && std::isspace(str[start])) start++;

	std::size_t end = str.size();
	while (end > start && std::isspace(str[end - 1])) end--;

	return str.substr(start, end - start);
}