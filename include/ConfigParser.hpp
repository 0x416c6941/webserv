#pragma once
#include "Webserv.hpp"
#include "ServerConfig.hpp"
#include "ConfigFile.hpp"
#include "ServerBuilder.hpp"

/**
 * @class ConfigParser
 * @brief Parses a web server configuration file (like NGINX).
 *
 * Extracts 'server { ... }' blocks, prepares them for interpretation,
 * and eventually builds ServerConfig objects.
 */
class ConfigParser {
	private:
		std::string 			_rawContent;		// Raw content of config file 
		std::vector<ServerConfig>	_servers;		// Final server(-s) configurations 
		std::vector<std::string>	_serverBlocks;		// Individual 'server { ... }' blocks

		/**
		 * @brief Removes all comments (starting with '#') from the config content.
		 * @param content String containing the configuration.
		 */
		void 				removeComments(std::string &content);
		
		/**
		 * @brief Extracts all 'server { ... }' blocks into _serverBlocks.
		 * @param content Full cleaned content of config file.
		 */
		void 				splitIntoServerBlocks(const std::string &content);

	
		// std::string 			trim(const std::string &str);

    		/**
		 * @brief Splits a server block into individual directives using ';' and '{...}' structure.
		 * @param block A trimmed server block string.
		 * @return Vector of trimmed directive strings.
		 */
		std::vector<std::string> 	splitDirectives(const std::string &block);


		/**
		 * @brief Removes empty and whitespace-only lines from a server block.
		 * @param block Input server block (modified in-place).
		 */
		void 				cleanLinesInPlace(std::string &block);


	public:
		/**
	 	* @brief Constructs the parser from config content.
	 	* @param content The raw config file content.
	 	*/
		ConfigParser(const std::string& content);
		~ConfigParser();

		/**
		 * @brief Runs the full parsing process: cleans, splits, and processes blocks.
		 * @throw ErrorException on parsing or format errors.
		 */
		void 				parse();  

/**
		 * @brief Finds the starting '{' index of a 'server' block.
		 * @param start Index from where to begin search.
		 * @param content Full config content.
		 * @return Index of opening '{'.
		 * @throw ErrorException if not found.
		 */
		size_t 				findStartServer(size_t start, const std::string &content);

		/**
		 * @brief Finds the corresponding closing '}' of a server block.
		 * @param start Index of the '{' character.
		 * @param content Full config content.
		 * @return Index of closing '}'.
		 * @throw ErrorException if block is unmatched.
		 */
		size_t 				findEndServer(size_t start, const std::string &content);

		
		// Getters
		const std::vector<ServerConfig>	&getServers() const;
		const std::vector<std::string>	&getServerBlocks() const;
	
	public:
		class ErrorException : public std::exception
		{
			private:
				std::string _message;
			public:
				ErrorException(std::string message) throw() {
					_message = "CONFIG PARSER ERROR: " + message;
				}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};