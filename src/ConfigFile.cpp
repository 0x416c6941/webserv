#include "../include/ConfigFile.hpp"

ConfigFile::ConfigFile(std::string path) : _path(path) {}

ConfigFile::~ConfigFile() {}

/**
 * @brief Validates whether the file exists and is readable.
 * 
 * Uses POSIX `access()` and `stat()` for checking.
 * 
 * @return int 0 if valid, non-zero if not.
 * @throws std::runtime_error If the file does not exist or is not readable.
 */
int ConfigFile::validateFile() {
	// Check if file exists and is readable
	if (access(_path.c_str(), R_OK) != 0) {
		throw std::runtime_error("File does not exist or is not readable: " + _path);
	}

	struct stat fileStat;
	if (stat(_path.c_str(), &fileStat) != 0) {
		throw std::runtime_error("Unable to retrieve file info: " + _path);
	}
	if (!S_ISREG(fileStat.st_mode)) {
		throw std::runtime_error("Path is not a regular file: " + _path);
	}
	return 0;
}

/**
 * @brief Reads and returns the contents of the config file.
 * 
 * @return std::string Content of the file.
 * @throws std::runtime_error If the file cannot be opened or read.
 */
std::string ConfigFile::readFile() {
	std::ifstream file(_path.c_str());

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open config file: " + _path);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	if (file.fail()) {
		throw std::runtime_error("Error while reading config file: " + _path);
	}

	return buffer.str();
}