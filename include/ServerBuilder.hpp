#pragma once
#include "Webserv.hpp"
// #include "ServerConfig.hpp"
#include "ConfigParser.hpp"

class ServerConfig;

/**
 * @class ServerBuilder
 * @brief Builds a ServerConfig from parsed configuration directives.
 */
class ServerBuilder {
private:

	static void 		handle_listen(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_server_name(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_root(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_index(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_host(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_mbs(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_autoindex(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_error_page(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
	static void 		handle_location(const std::vector<std::string>& parameters, ServerConfig& server_cfg);
public:
    	/**
    	* @brief Parses directives and builds a validated ServerConfig.
    	* @param directives List of config lines (e.g., "listen 8080;").
    	* @return Fully populated ServerConfig object.
    	* @throws ConfigParser::ErrorException if any directive is invalid.
    	*/
   	static ServerConfig 	build(const std::vector<std::string>& directives);

};