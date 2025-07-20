#pragma once

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/epoll.h>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include <netinet/in.h>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ctime>
#include <iomanip>


#define DEBUG 0

#define EPOLL_MAX_EVENTS 1024

#define DEFAULT_CONTENT_LENGTH 1048576
#define MAX_CONTENT_LENGTH 1073741824 	//1GB
#define MAX_HEADER_CONTENT_LENGTH 40960 //5*8k
#define DEFAULT_LARGE_CLIENT_HEADER_BUFFERS 4
#define DEFAULT_LARGE_CLIENT_HEADER_BUFFER_SIZE 8096 //8k
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

// We don't support custom server names in configs.
// So the server name will always be static and it will be this:
#define SERVER_NAME "hlyshchu_asagymba"

// Utils.

/**
* @brief Trims whitespace from both ends of the input string.
* @param str Input string.
* @return Trimmed result.
* @note This method could be made static.
*/
std::string 	trim(const std::string& str);

/**
 * @brief Prints a standard log message to std::clog with a green "Warning" label.
 *
 * @param desc      Description or prefix for the message.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_log(const std::string &desc, const std::string &line, const std::string &opt_desc);

/**
 * @brief Prints an error message to std::cerr with a red "Warning" label.
 *
 * @param desc      Description or prefix for the error.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_err(const std::string &desc, const std::string &line, const std::string &opt_desc);

/**
 * @brief Prints a warning message to std::clog with a yellow "Warning" label.
 *
 * @param desc      Description or prefix for the warning.
 * @param line      Line with issue.
 * @param opt_desc  Optional description or suffix to append.
 */
void print_warning(const std::string &desc, const std::string &line, const std::string &opt_desc);

/**
 * @brief	check if \p path exists
 * @param	path
 * @return	true, if yes;
 * 		false otherwise.
 */
bool pathExists(const std::string &path);

bool isDirectory(const std::string &path);
bool isRegFile(const std::string &path);

std::string to_string(uint16_t value);
std::string to_string(uint32_t value);
std::string to_string(int value);
std::string to_string(size_t value);

uint64_t 	validateGetMbs(std::string param);

/**
 * Read the file at \p path.
 * @throw	std::ios_base::failure	Got IO error.
 * @param	path	File's directory.
 * @return	Read file.
 */
std::string read_file(const std::string &path);

/**
 * Append file stored at \p path with \p with_what.
 * If file at \p path doesn't exist, it will be created.
 * @throw	std::ios_base::failure	Got IO error.
 * @param	path		File to append.
 * @param	with_what	Content to append to \p path.
 */
void append_file(const std::string &path, const std::string &with_what);

/**
 * Gets the extension in lowercase of \p path.
 * @param	path	Path to some file.
 * @return	If \p path does contain some extension,
 * 		that extension in the lowercase and with the
 * 		beginning '.'.
 * @return	Empty string, if \p path doesn't have any extension
 * 		or if '.' is the last character in it.
 */
std::string get_file_ext(const std::string &path);

/**
 * Gets the mime type depending on extension
 * of the file in \p path.
 * @param	path	Path to a file.
 * @return	Appropriate mime type for file in \p path.
 */
std::string get_mime_type(const std::string &path);

/**
 * Our implementation of inet_ntop() for AF_INET.
 * @param	src	Pointer to uint32_t from `struct sockaddr_in`.
 * @param	dst	Where to write the text form of \p src.
 * @param	size	Size in bytes of \p dst.
 * @return	\p dst, if everything went fine.
 * @return	NULL, if \p size isn't sufficient to store text form of \p src
 * 		in \p dst (errno will be set to ENOSPC in this case).
 */
const char * our_inet_ntop4(const void * src, char * dst, socklen_t size);

// Errors.
std::string getReasonPhrase(int status_code);
std::string generateErrorPage(int status_code);
std::string generateErrorHeader(int status_code, size_t content_length);
std::string generateErrorBody(int status_code);

// Debug.
class ServerConfig;
void printServerConfig(const ServerConfig& config);
std::string escape_string(const std::string &input);

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
bool validateDirPath(const std::string &path);
