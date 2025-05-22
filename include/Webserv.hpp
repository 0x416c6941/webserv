#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>


#define MAX_CONTENT_LENGTH 30000000
#define DEBUG false

/// Utils

/**
* @brief Trims whitespace from both ends of the input string.
* @param str Input string.
* @return Trimmed result.
* @note This method could be made static.
*/
std::string 	trim(const std::string& str);
