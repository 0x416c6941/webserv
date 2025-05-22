#pragma once
#include "Webserv.hpp"

/**
 * @class ConfigFile
 * @brief Handles loading and validating a configuration file.
 * 
 * This class stores the path to the configuration file, validates its accessibility,
 * and reads its content. It acts as a simple helper utility for working with config files.
 */
class ConfigFile
{
private:
	std::string 	_path;  // Absolute or relative path to the config file.
public:
	ConfigFile(std::string const path);
	~ConfigFile();

	/**
	 * @brief Validates if the config file exists and is readable.
	 * 
	 */
	void 		validateFile();

	/**
	 * @brief Reads the content of the given file.
	 * 
	 * @param path Path to the file to read.
	 * @return std::string Contents of the file.
	 */
	std::string	readContent();

};
