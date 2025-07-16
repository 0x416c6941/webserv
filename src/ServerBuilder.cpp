#include "../include/ServerBuilder.hpp"

/**
 * @brief Splits a directive string into individual tokens.
 *
 * Tokens are separated by whitespace, and semicolons are treated as independent tokens
 * (e.g., "index index.html;" → {"index", "index.html", ";"}).
 *
 * @param directive A string representing a single configuration line.
 * @return std::vector<std::string> A list of tokenized strings.
 */
std::vector<std::string> splitParameters(const std::string &directive) {
    std::vector<std::string> tokens;
    std::istringstream iss(directive);
    std::string word;

    while (iss >> word) {
        size_t semicolonPos = word.find(';');

        if (semicolonPos != std::string::npos) {
            // Split word before and after semicolon
            std::string before = word.substr(0, semicolonPos);
            if (!before.empty())
                tokens.push_back(before);
            tokens.push_back(";");

            // If there is more content after ';' (e.g., abc;xyz)
            if (word.length() > semicolonPos + 1) {
                // Optional: recursively parse
                tokens.push_back(word.substr(semicolonPos + 1));
            }
        }
        else {
            tokens.push_back(word);
        }
    }

    return tokens;
}


/**
 * @brief Processes the 'host' directive for a server block.
 *
 * Expected format: `host <ip>;`
 *
 * @param parameters Tokenized directive line.
 * @param server_cfg The server configuration object being constructed.
 * @throws ConfigParser::ErrorException On syntax or IP validation failure.
 */
void ServerBuilder::handle_host(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() != 3 || parameters[2] != ";")
		throw ConfigParser::ErrorException("Invalid syntax for host directive");

	const std::string& ip = parameters[1];

	struct in_addr addr;
	if (ip != "localhost" && inet_pton(AF_INET, ip.c_str(), &addr) != 1)
		throw ConfigParser::ErrorException("Invalid IPv4 address: " + ip);

	// Optional: map "localhost" to 127.0.0.1
	std::string resolvedIp = (ip == "localhost") ? "127.0.0.1" : ip;

	if (server_cfg.alreadyAddedHost(resolvedIp))
		throw ConfigParser::ErrorException("Duplicate host: " + resolvedIp);

	server_cfg.addHost(resolvedIp);
}



/**
 * @brief Handles the 'root' directive for server-wide root path.
 *
 * Format: `root <path>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On incorrect syntax.
 */
void ServerBuilder::handle_root(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    	if (parameters.size() != 3 || parameters[2] != ";")
        	throw ConfigParser::ErrorException("Invalid syntax for 'root' directive");
	if (!pathExists(parameters[1])) {
		print_warning ("root path '", parameters[1], "' does not exist at parse time.");
	}
    	server_cfg.setRoot(parameters[1]);
}

// /**
//  * @brief Processes the 'server_name' directive.
//  *
//  * Format: `server_name <name>;`
//  *
//  * @param parameters Tokenized directive.
//  * @param server_cfg Server configuration to update.
//  * @throws ConfigParser::ErrorException On incorrect syntax.
//  */
// void ServerBuilder::handle_server_name(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
// 	if (parameters.size() < 3 || parameters.back() != ";")
// 		throw ConfigParser::ErrorException("Invalid syntax for 'server_name' directive");

// 	// Add each name (parameters[1] to parameters[n - 2])
// 	for (size_t i = 1; i < parameters.size() - 1; ++i) {
// 		const std::string& name = parameters[i];

// 		// Must not be empty, no spaces
// 		if (name.empty() || name.find(' ') != std::string::npos)
// 			throw ConfigParser::ErrorException("Invalid server_name: '" + name + "'");

// 		server_cfg.addServerName(name);
// 	}
// }


/**
 * @brief Handles the 'index' directive specifying default index files.
 *
 * Format: `index <file1> <file2> ... ;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On incorrect syntax.
 */
void ServerBuilder::handle_index(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() < 3 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for 'index' directive");

	// Clear existing index list
	server_cfg.resetIndex();

	// Add all index files
	for (size_t i = 1; i < parameters.size() - 1; ++i) {
		if (parameters[i].empty())
			throw ConfigParser::ErrorException("Empty value in 'index' directive");
		server_cfg.addIndex(parameters[i]);
	}
}

/**
 * @brief Processes the 'autoindex' directive (on/off).
 *
 * Format: `autoindex on;` or `autoindex off;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On invalid value or syntax.
 */
void ServerBuilder::handle_autoindex(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() < 3 || parameters[2] != ";")
		throw ConfigParser::ErrorException("Invalid syntax for autoindex directive");

	const std::string& value = parameters[1];
	if (value == "on")
		server_cfg.setAutoindex(true);
	else if (value == "off")
		server_cfg.setAutoindex(false);
	else
		throw ConfigParser::ErrorException("Invalid value for autoindex: " + value);
}

/**
 * @brief Handles 'client_max_body_size' directive.
 *
 * Format: `client_max_body_size <size>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On syntax error.
 */
void ServerBuilder::handle_mbs(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() != 3 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for client_max_body_size directive");

	const std::string& param = parameters[1];
	if (param.empty())
		throw ConfigParser::ErrorException("client_max_body_size cannot be empty");
	uint64_t 	finalSize = validateGetMbs(param);
	server_cfg.setClientMaxBodySize(finalSize);
}

/**
 * @brief Handles the 'large_client_header_buffers' directive.
 *
 * This directive specifies the maximum number and size of buffers used
 * for reading large client request headers. It is typically placed in the
 * HTTP block of an NGINX configuration.
 *
 * Format: `large_client_header_buffers <number_of_buffers> <buffer_size>;`
 *
 * Example: `large_client_header_buffers 4 16k;`
 *
 * Validates that:
 * - The number of buffers is a numeric value between 1 and 1024.
 * - The buffer size is a valid size string (e.g., "8k", "16k", "1m").
 * - The total size (`number * size`) does not exceed MAX_CONTENT_LENGTH.
 *
 * @param parameters Tokenized directive as parsed from the configuration.
 * @param server_cfg Server configuration object to apply the settings to.
 *
 * @throws ConfigParser::ErrorException If syntax is invalid or values exceed limits.
 */
void ServerBuilder::handle_large_client_header_buffers(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() != 4 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for large_client_header_buffers directive");

	const std::string& buffer_nbr = parameters[1];
	const std::string& buffer_size = parameters[2];

	if (buffer_nbr.empty() || buffer_size.empty())
		throw ConfigParser::ErrorException("large_client_header_buffers values cannot be empty");
	for (std::string::const_iterator it = buffer_nbr.begin(); it != buffer_nbr.end(); ++it) {
		if (!isdigit(*it))
			throw ConfigParser::ErrorException("Buffer count must be a numeric value");
	}
	std::istringstream iss(buffer_nbr);
	uint32_t bufferCount = 0;
	iss >> bufferCount;
	if (iss.fail() || bufferCount == 0 || bufferCount > 1024)
		throw ConfigParser::ErrorException("Buffer count must be between 1 and 1024");
	uint64_t 	finalBufferSize = validateGetMbs(buffer_size);

	if (bufferCount * finalBufferSize > MAX_HEADER_CONTENT_LENGTH)
		throw ConfigParser::ErrorException("Total buffer size exceeds 40k limit");
	server_cfg.setLargeClientHeaderBuffers(bufferCount, finalBufferSize);
}

/**
 * @brief Processes 'error_page' directive mapping codes to pages.
 *
 * Format: `error_page <code1> <code2> ... <file>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On invalid codes or syntax.
 */
void ServerBuilder::handle_error_page(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
        enum { MIN_EXPECTED_PARAMS = 4 };
        if (parameters.size() < MIN_EXPECTED_PARAMS || parameters.back() != ";")
                throw ConfigParser::ErrorException("Invalid syntax for error_page directive");

        const std::string& page_path = parameters[parameters.size() - 2];

        for (size_t i = 1; i < parameters.size() - 2; ++i) {
                const std::string& code_str = parameters[i];
                for (size_t j = 0; j < code_str.size(); ++j) {
                        if (!std::isdigit(code_str[j]))
                                throw ConfigParser::ErrorException("Invalid error code: " + code_str);
                }

                int code = std::atoi(code_str.c_str());
                if (code < 400 || code > 599)
                        throw ConfigParser::ErrorException("Error code out of range: " + code_str);

                server_cfg.setErrorPage(code, page_path);
        }
}


/**
 * @brief Handles the 'listen' directive defining the port.
 *
 * Format: `listen <port>;` or `listen <ip:port>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On invalid syntax or value.
 */
void ServerBuilder::handle_listen(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    	if (parameters.size() != 3 || parameters[2] != ";") {
        	throw ConfigParser::ErrorException("Invalid syntax for 'listen': expected format 'listen <port>;' or 'listen <ip:port>;'");
   	}

	std::string listen_value = parameters[1];
    	std::string host, port_str;

    	size_t colon_pos = listen_value.find(':');
    	if (colon_pos != std::string::npos) {
        	// Case: explicit host:port
        	host = listen_value.substr(0, colon_pos);
		port_str = listen_value.substr(colon_pos + 1);
		if (host == "localhost") {
			host = "127.0.0.1";
		} else {
			struct in_addr addr;
			if (inet_pton(AF_INET, host.c_str(), &addr) != 1) {
				throw ConfigParser::ErrorException("Invalid IPv4 address in 'listen' directive: " + host);
			}
		}
    	}
    	else {
        // Case: just port
        	port_str = listen_value;
    	}

    	// Convert and validate port
	char* end;
	unsigned long port = std::strtoul(port_str.c_str(), &end, 10);
	if (*end != '\0' || port == 0 || port > 65535) {
		throw ConfigParser::ErrorException("Invalid port number in 'listen' directive: " + port_str);
	}
	uint16_t port_val = static_cast<uint16_t>(port);

	if (colon_pos != std::string::npos) {
		server_cfg.addListenEndpoint(std::make_pair(host, port_val));
	} else {
		server_cfg.addPort(port_val);
	}
}

/**
 * @brief Handles the 'root' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to skip parsed elements.
 * @throws ConfigParser::ErrorException if syntax is invalid.
 */
static void handle_location_root(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
    	if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        	throw ConfigParser::ErrorException("Invalid root directive in location block");
    	loc.setRootLocation(tokens[i + 1]);
    	i += 2;
}

/**
 * @brief Handles the 'index' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to point to next directive.
 * @throws ConfigParser::ErrorException if the directive is malformed or terminator is missing.
 */
static void handle_location_index(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
	++i;
	while (i < tokens.size()) {
		if (tokens[i] == ";")
			return;
		loc.addIndexLocation(tokens[i]);
		++i;
	}
	throw ConfigParser::ErrorException("Missing ';' after index directive in location block");
}

/**
 * @brief Handles the 'autoindex' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to skip parsed elements.
 * @throws ConfigParser::ErrorException if value is missing or syntax is incorrect.
 */
static void handle_location_autoindex(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
	if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
		throw ConfigParser::ErrorException("Invalid syntax for autoindex directive in location block");

	const std::string& value = tokens[i + 1];
	if (value == "on")
		loc.setAutoindex(true);
	else if (value == "off")
		loc.setAutoindex(false);
	else
		throw ConfigParser::ErrorException("Invalid value for autoindex: " + value);
	i += 2;
}


/**
 * @brief Handles the 'allow_methods' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to point to the next directive.
 * @throws ConfigParser::ErrorException if an unknown method is encountered or ';' is missing.
 */
static void handle_location_allow_methods(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
	static std::set<std::string> kAllowedMethods;
	if (kAllowedMethods.empty()) {
		kAllowedMethods.insert("GET");
		kAllowedMethods.insert("POST");
		kAllowedMethods.insert("DELETE");
		kAllowedMethods.insert("PUT");
	}

	loc.resetMethods();

	++i;
	while (i < tokens.size() && tokens[i] != ";") {
		if (kAllowedMethods.find(tokens[i]) == kAllowedMethods.end()) {
			throw ConfigParser::ErrorException("Invalid HTTP method: " + tokens[i]);
		}
		loc.addMethod(tokens[i]);
		++i;
	}

	if (i >= tokens.size() || tokens[i] != ";") {
		throw ConfigParser::ErrorException("Missing ';' after allow_methods directive");
	}
}

/**
 * @brief Handles the 'alias' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to skip parsed elements.
 * @throws ConfigParser::ErrorException if syntax is invalid.
 */
static void handle_location_alias(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
	if (i + 2 >= tokens.size())
		throw ConfigParser::ErrorException("Incomplete alias directive in location block");

	if (tokens[i + 2] != ";")
		throw ConfigParser::ErrorException("Missing ';' after alias directive");

	const std::string& aliasPath = tokens[i + 1];
	if (aliasPath.empty())
		throw ConfigParser::ErrorException("Alias path cannot be empty");

	loc.setAlias(aliasPath);
	i += 2;
}


/**
 * @brief Handles the 'cgi_path' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to point to the next directive.
 * @throws ConfigParser::ErrorException if the path or terminator is missing.
 */
static void handle_location_cgi_path(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
    ++i;
    if (i >= tokens.size())
        throw ConfigParser::ErrorException("Missing value(s) for cgi_path directive");

    while (i < tokens.size() && tokens[i] != ";") {
        loc.addCgiPath(tokens[i]);
        ++i;
    }

    if (i >= tokens.size() || tokens[i] != ";")
        throw ConfigParser::ErrorException("Missing ';' after cgi_path directive");
}

/**
 * @brief Handles the 'cgi_ext' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to point to the next directive.
 * @throws ConfigParser::ErrorException if ';' is missing after the extensions.
 */
static void handle_location_cgi_ext(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
    	++i;
	if (i >= tokens.size())
		throw ConfigParser::ErrorException("Missing value(s) for cgi_path directive");
    	bool valueAdded = false;
	while (i < tokens.size()) {
		if (tokens[i] == ";")
			break;

		loc.addCgiExtension(tokens[i]);
		valueAdded = true;
		++i;
	}

	if (!valueAdded)
		throw ConfigParser::ErrorException("cgi_path directive requires at least one value");

	if (i >= tokens.size() || tokens[i] != ";")
		throw ConfigParser::ErrorException("Missing ';' after cgi_path directive");
}

/**
 * @brief Handles the 'client_max_body_size' directive inside a location block.
 *
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to skip parsed elements.
 * @throws ConfigParser::ErrorException if syntax is invalid.
 */
static void handle_location_client_max_body_size(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
    	if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        	throw ConfigParser::ErrorException("Invalid client_max_body_size directive in location block");
	uint64_t value = validateGetMbs(tokens[i+1]);
    	loc.setMaxBodySize(value);
    	i += 2;
}

static void handle_location_upload_path(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
	 // Check syntax: upload_path <path> ;
	if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
		throw ConfigParser::ErrorException("Invalid upload_path directive in location block");

	const std::string& path = tokens[i + 1];
	if (path.empty())
		throw ConfigParser::ErrorException("upload_path cannot be empty");

	if (!pathExists(path))
		print_warning("upload_path '", path, "' does not exist at parse time.");

	loc.setUploadPath(path);
	i += 2;
}

static void handle_location_error_page(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
        if (i + 3 >= tokens.size() || tokens[i + 3] != ";")
                throw ConfigParser::ErrorException("Invalid error_page directive in location block");

        const std::string& code_str = tokens[i + 1];
        for (size_t j = 0; j < code_str.size(); ++j) {
                if (!std::isdigit(code_str[j]))
                        throw ConfigParser::ErrorException("Invalid error code: " + code_str);
        }

        int code = std::atoi(code_str.c_str());
        if (code < 400 || code > 599)
                throw ConfigParser::ErrorException("Error code out of range: " + code_str);

        const std::string& path = tokens[i + 1 + 1]; // second token after directive name
        loc.setErrorPage(code, path);

        i += 3;
}

/**
 * @brief Returns a map of supported location directive handlers.
 *
 * @return const std::map<std::string, LocationHandler>&
 *         A static map linking directive names to their handler functions.
 */
typedef void (*LocationHandler)(Location&, const std::vector<std::string>&, size_t&);

static const std::map<std::string, LocationHandler>& getLocationHandlers() {
    static std::map<std::string, LocationHandler> handlers;
    if (handlers.empty()) {
        handlers["root"] = handle_location_root;
        handlers["index"] = handle_location_index;
        handlers["autoindex"] = handle_location_autoindex;
        handlers["allow_methods"] = handle_location_allow_methods;
        handlers["alias"] = handle_location_alias;
        handlers["cgi_path"] = handle_location_cgi_path;
        handlers["cgi_ext"] = handle_location_cgi_ext;
        handlers["client_max_body_size"] = handle_location_client_max_body_size;
	handlers["upload_path"] = handle_location_upload_path;
	handlers["error_page"] = handle_location_error_page;
    }
    return handlers;
}


/**
 * @brief Handles a 'location' block and its nested directives.
 *
 * Format:
 * ```
 * location <path> {
 *     directive1 ...
 *     directive2 ...
 * }
 * ```
 *
 * @param parameters Full token list for the location block.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On unknown directive or syntax error.
 */
void ServerBuilder::handle_location(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    	if (parameters.size() < 3 || parameters[0] != "location" || parameters[2] != "{")
        	throw ConfigParser::ErrorException("Invalid or missing URI for location block");

    	Location location;
    	location.setPath(parameters[1]);

    	const std::map<std::string, LocationHandler>& handlers = getLocationHandlers();

    	for (size_t i = 3; i < parameters.size(); ++i) {
        	if (parameters[i] == "}") break;

        	std::map<std::string, LocationHandler>::const_iterator it = handlers.find(parameters[i]);
        	if (it == handlers.end()) {
            		// throw ConfigParser::ErrorException("Unknown directive: " + parameters[i]);
			print_warning("Unknown directive: '", parameters[i], "' in location block. ");
			// Can't skip cause it doesn't garanteed that we will find '}' after unknown directive
			// Skip unknown directive arguments until ';' or '}'
			// while (i < parameters.size() && parameters[i] != ";" && parameters[i] != "}") {
			// 	++i;
			// }
			// if (i < parameters.size() && parameters[i] == "}")
			// 	break; // Exit if we hit the end of the block
        	}
		else {
        		LocationHandler handler = it->second;
        		handler(location, parameters, i);
		}
    	}
	location.validateLocation();
    	server_cfg.addLocation(location);
}

/**
 * @brief Retrieves the handler function for a directive.
 *
 * @param directive Directive keyword (e.g., "listen", "host").
 * @return HandlerFunc Corresponding member function pointer, or NULL if not found.
 */
HandlerFunc ServerBuilder::getHandler(const std::string& directive) {
	static std::map<std::string, HandlerFunc> handlers;
	if (handlers.empty()) {
		handlers["listen"] = &ServerBuilder::handle_listen;
		handlers["host"] = &ServerBuilder::handle_host;
		// handlers["server_name"] = &ServerBuilder::handle_server_name;
		handlers["root"] = &ServerBuilder::handle_root;
		handlers["client_max_body_size"] = &ServerBuilder::handle_mbs;
		handlers["autoindex"] = &ServerBuilder::handle_autoindex;
		handlers["index"] = &ServerBuilder::handle_index;
		handlers["error_page"] = &ServerBuilder::handle_error_page;
		handlers["location"] = &ServerBuilder::handle_location;
		handlers["large_client_header_buffers"] = &ServerBuilder::handle_large_client_header_buffers;
	}

	std::map<std::string, HandlerFunc>::const_iterator it = handlers.find(directive);
	return (it != handlers.end()) ? it->second : NULL;
}


/**
 * @brief Main entry point to build a ServerConfig from parsed directive lines.
 *
 * @param directives List of raw configuration lines (one line = one directive block).
 * @return ServerConfig Fully populated server configuration object.
 * @throws ConfigParser::ErrorException On unrecognized directive or malformed lines.
 */
ServerConfig ServerBuilder::build(const std::vector<std::string>& directives) {
	ServerConfig server_cfg;

	for (size_t line = 0; line < directives.size(); ++line) {
		std::vector<std::string> tokens = splitParameters(directives[line]);
		const std::string& directive = tokens[0];
		HandlerFunc handler = getHandler(directive);
		if (!handler) {
			// throw ConfigParser::ErrorException("Unknown directive: '" + directive + "'");
			print_warning("Unknown directive: '", directive, ".");
		}
		else {
			handler(tokens, server_cfg);
		}
	}
	// CGI paths count must correspond to count of CGI extensions.
	// Basically, we provide a path to a handler to each CGI extension type.
	for (std::vector<Location>::const_iterator it = server_cfg.getLocations().begin();
		it != server_cfg.getLocations().end(); ++it)
	{
		if (it->getCgiPath().size() != it->getCgiExtension().size())
		{
			throw ConfigParser::ErrorException(std::string("ServerBuilder::build(): ")
					+ "Location \"" + it->getPath()
					+ "\": amount of CGI paths isn't equal to "
					+ "amount of CGI extensions provided.");
		}
	}
	return server_cfg;
}
