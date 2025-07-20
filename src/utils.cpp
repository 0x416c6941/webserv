#include "Webserv.hpp"
#include "ConfigParser.hpp"
#include <inttypes.h>	// <cinttypes> is available from C++11 onwards, but we use C++98.
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <arpa/inet.h>
#include <cstddef>
#include <cerrno>
#include <cstdio>

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

bool		isRegFile(const std::string &path) {
	struct stat sb;

	if (stat(path.c_str(), &sb) != 0) {
		return false;
	}
	return S_ISREG(sb.st_mode);
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

std::string read_file(const std::string &path)
{
	std::ifstream file;
	std::string line;
	std::ostringstream ret;

	file.open(path.c_str());
	if (!file.is_open())
	{
		throw std::ios_base::failure(std::string("read_file(): Couldn't open file: ")
				+ path + '.');
	}
	for (;;)
	{
		std::getline(file, line);
		if (file.fail() && !file.eof())
		{
			throw std::ios_base::failure(std::string("read_file: ")
					+ path + " is corrupted.");
		}
		ret << line;
		if (file.eof())
		{
			break;
		}
		ret << '\n';
		line.clear();
	}
	return ret.str();
}

void append_file(const std::string &path, const std::string &with_what)
{
	std::ofstream file;

	file.open(path.c_str(), std::ios_base::app);	// Open for appending.
	if (!file.is_open())
	{
		throw std::ios_base::failure(std::string("append_file(): Couldn't open file: ")
				+ path + '.');
	}
	file << with_what;
	if (!file.good())
	{
		throw std::ios_base::failure(std::string("append_file(): Couldn't append file: ")
				+ path + '.');
	}
}

std::string get_file_ext(const std::string &path)
{
	std::string::size_type dot = path.find_last_of('.');
	std::string ret;

	if (dot == std::string::npos
		|| dot + 1 >= path.length())
	{
		return "";
	}
	ret = path.substr(dot);
	for (size_t i = 0; i < ret.size(); i++)
	{
		ret[i] = std::tolower(static_cast<unsigned char> (ret[i]));
	}
	return ret;
}

std::string get_mime_type(const std::string &path)
{
	static std::map<std::string, std::string> mime_map;
	std::string ext;

	if (mime_map.empty())
	{
		mime_map.insert(std::make_pair(".html", "text/html"));
		mime_map.insert(std::make_pair(".htm", "text/html"));
		mime_map.insert(std::make_pair(".css", "text/css"));
		mime_map.insert(std::make_pair(".js", "application/javascript"));
		mime_map.insert(std::make_pair(".png", "image/png"));
		mime_map.insert(std::make_pair(".jpg", "image/jpeg"));
		mime_map.insert(std::make_pair(".jpeg", "image/jpeg"));
		mime_map.insert(std::make_pair(".webp", "image/webp"));
		mime_map.insert(std::make_pair(".gif", "image/gif"));
		mime_map.insert(std::make_pair(".svg", "image/svg+xml"));
		mime_map.insert(std::make_pair(".json", "application/json"));
		mime_map.insert(std::make_pair(".pdf", "application/pdf"));
		mime_map.insert(std::make_pair(".txt", "text/plain"));
		mime_map.insert(std::make_pair(".xml", "application/xml"));
	}
	ext = get_file_ext(path);
	std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
	if (it != mime_map.end())
	{
		return it->second;
	}
	return "application/octet-stream";
}

const char * our_inet_ntop4(const void * src, char * dst, socklen_t size)
{
	// "255.255.255.255" => 15 bytes + 1 for '\0'.
	const size_t MIN_LENGTH = 16;
	const unsigned char * b_src;

	if (static_cast<size_t> (size) < MIN_LENGTH)
	{
		errno = ENOSPC;
		return NULL;
	}
	// Thanks to OpenBSD!
	b_src = reinterpret_cast<const unsigned char *> (src);
	(void) snprintf(dst, static_cast<size_t> (size),
		"%u.%u.%u.%u", b_src[0], b_src[1], b_src[2], b_src[3]);
	return dst;
}

/**
 * @brief Validates whether a given path is a valid, accessible directory path.
 *
 * This function checks that the input path:
 *   - is not empty,
 *   - ends with a '/',
 *   - exists in the filesystem,
 *   - is a directory,
 *   - and has both read and execute permissions.
 *
 * @param path The directory path to validate.
 * @return true if the path is non-empty, ends with '/', exists,
 *         is a directory, and is accessible (read + execute).
 * @return false otherwise.
 */
bool validateDirPath(const std::string &path)
{
        return !path.empty()
                && pathExists(path)
                && isDirectory(path)
                && access(path.c_str(), R_OK | X_OK) == 0;
}
