#include "../include/Webserv.hpp"
#include "../include/ConfigParser.hpp"
#include <inttypes.h>	// <cinttypes> is available from C++11 onwards, but we use C++98.

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

/**
 * @brief Prints a standard log message to std::clog with a green "Warning" label.
 *
 * @param desc      Description or prefix for the message.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_log(const std::string &desc, const std::string &line, const std::string &opt_desc)
{
	std::cerr << GREEN << "Log: " << RESET << desc << line << opt_desc << RESET << std::endl;
}

/**
 * @brief Prints an error message to std::cerr with a red "Warning" label.
 *
 * @param desc      Description or prefix for the error.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_err(const std::string &desc, const std::string &line, const std::string &opt_desc)
{
	std::cerr << RED << "Error: " << desc << line << opt_desc << RESET << std::endl;
}

/**
 * @brief Prints a warning message to std::clog with a yellow "Warning" label.
 *
 * @param desc      Description or prefix for the warning.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_warning(const std::string &desc, const std::string &line, const std::string &opt_desc)
{
	std::clog << YELLOW << "Warning: " << desc << line << opt_desc << RESET << std::endl;
}

bool 		pathExists(const std::string &path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

bool		isDirectory(const std::string &path) {
	struct stat sb;

	if (stat(path.c_str(), &sb) != 0) {
		return false;
	}
	return S_ISDIR(sb.st_mode);
}

/**
 * @warning user responsible for providing non-empty param
 * @param param
 * @return uint64_t
 */
uint64_t 	validateGetMbs(std::string param) {
	if (param.empty())
		throw ConfigParser::ErrorException("client or header max_body_size  cannot be empty");

	std::string numericPart = param;
	char suffix = param[param.size() - 1];
	unsigned long multiplier = 1;

	if (suffix == 'K' || suffix == 'k'  || suffix == 'M' || suffix == 'm' || suffix == 'G' || suffix == 'g') {
		numericPart = param.substr(0, param.size() - 1);
		if (suffix == 'K' || suffix == 'k') multiplier = 1024UL;
		else if (suffix == 'M' || suffix == 'm') multiplier = 1024UL * 1024UL;
		else if (suffix == 'G' || suffix == 'g') multiplier = 1024UL * 1024UL * 1024UL;
	}

	std::istringstream iss(numericPart);
	unsigned long size = 0;
	iss >> size;

	if (iss.fail() || !iss.eof())
		throw ConfigParser::ErrorException("Invalid number in client or header max_body_size: " + param);

	// Overflow check before multiplication
	if (size * multiplier / multiplier != size)
		throw ConfigParser::ErrorException("client or header max_body_size too large: " + param);
	size *= multiplier;

	if (size > MAX_CONTENT_LENGTH)
		throw ConfigParser::ErrorException("client or header max_body_size exceeds maximum allowed (1GB): " + param);
	return (size);
}


std::string to_string(uint16_t value) {
	enum { BUF_SIZE = 6 };	// "65535" + '\0'.
	char buf[BUF_SIZE];
	std::sprintf(buf, "%u", value);
	return std::string(buf);
}

std::string to_string(uint32_t value) {
    	enum { BUF_SIZE = 11 }; // Max uint32_t = "4294967295" + '\0'
    	char buf[BUF_SIZE];
    	std::sprintf(buf, "%u", value);
    	return std::string(buf);
}


std::string to_string(int value) {
	enum { BUF_SIZE = 12 };	// "-2147483648" + '\0'.
	char buf[BUF_SIZE];
	std::sprintf(buf, "%d", value);
	return std::string(buf);
}


std::string to_string(size_t value) {
	enum { BUF_SIZE = 21 }; // "18446744073709551615" + '\0'.
	char buf[BUF_SIZE];
	std::sprintf(buf, "%" PRIuMAX, value);
	return std::string(buf);
}
