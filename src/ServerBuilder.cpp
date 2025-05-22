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
            std::string after = word.substr(semicolonPos + 1);
            if (!after.empty())
                tokens.push_back(after);  // optional: recursively parse
        } else {
            tokens.push_back(word);
        }
    }

    return tokens;
}

/**
 * @brief Validates an IPv4 address format (basic check).
 * 
 * @param ip A string representing the IP address.
 * @return true If the IP is a valid IPv4 address.
 * @return false Otherwise.
 */
static bool isValidIPv4(const std::string& ip) {
	std::stringstream ss(ip);
	std::string token;
	int segments = 0;

	while (std::getline(ss, token, '.')) {
		if (++segments > 4) return false;
		int val = std::atoi(token.c_str());
		if (val < 0 || val > 255) return false;
	}
	return segments == 4;
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
	if (!isValidIPv4(ip))
		throw ConfigParser::ErrorException("Invalid IPv4 address: " + ip);

	server_cfg.setHost(ip);
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

    server_cfg.setRoot(parameters[1]);
}

/**
 * @brief Processes the 'server_name' directive.
 *
 * Format: `server_name <name>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On incorrect syntax.
 */
void ServerBuilder::handle_server_name(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    if (parameters.size() != 3 || parameters[2] != ";")
        throw ConfigParser::ErrorException("Invalid syntax for 'server_name' directive");
    server_cfg.setServerName(parameters[1]);
}

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

    std::string index_files;
    for (size_t i = 1; i < parameters.size() - 1; ++i) {
        if (!index_files.empty()) index_files += " ";
        index_files += parameters[i];
    }

    server_cfg.setIndex(index_files);
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
	if (parameters.size() < 3 || parameters.back() != ";")
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
	if (parameters.size() < 3 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for client_max_body_size directive");
	server_cfg.setClientMaxBodySize(parameters[1]);
	
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
	if (parameters.size() < 3 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for error_page directive");

	for (size_t i = 1; i < parameters.size() - 2; ++i) {
		int code = std::atoi(parameters[i].c_str());
		if (code < 400 || code > 599)
			throw ConfigParser::ErrorException("Invalid error code: " + parameters[i]);
		server_cfg.setErrorPage(code, parameters[parameters.size() - 2]);
	}
}

/**
 * @brief Handles the 'listen' directive defining the port.
 *
 * Format: `listen <port>;`
 *
 * @param parameters Tokenized directive.
 * @param server_cfg Server configuration to update.
 * @throws ConfigParser::ErrorException On invalid port or duplicate.
 */
void ServerBuilder::handle_listen(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    if (parameters.size() != 3 || parameters[2] != ";")
        throw ConfigParser::ErrorException("Invalid syntax for 'listen': expected format 'listen <port>;'");

    if (server_cfg.getPort() != 0)
        throw ConfigParser::ErrorException("Duplicate 'listen' directive");

    char* end;
    unsigned long port = std::strtoul(parameters[1].c_str(), &end, 10);

    if (*end != '\0' || port > 65535)
        throw ConfigParser::ErrorException("Invalid port number in 'listen' directive");

    server_cfg.setPort(static_cast<uint16_t>(port));
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
    while (i < tokens.size() && tokens[i] != ";")
        loc.addIndexLocation(tokens[i++]);
    if (i >= tokens.size())
        throw ConfigParser::ErrorException("Missing ';' after index directive");
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
    if (i + 1 >= tokens.size())
        throw ConfigParser::ErrorException("Missing value for autoindex directive");
    loc.setAutoindex(tokens[i + 1]);
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
    loc.resetMethode();  

    ++i;
    while (i < tokens.size() && tokens[i] != ";") {
        const std::string& method = tokens[i];
        if (method == "GET" || method == "POST" || method == "DELETE" || method == "PUT" || method == "HEAD") {
            loc.addMethode(method);
        } else {
            throw ConfigParser::ErrorException("Unknown method: " + method);
        }
        ++i;
    }
    if (i >= tokens.size())
        throw ConfigParser::ErrorException("Missing ';' after allow_methods directive");
}

/**
 * @brief Handles the 'return' directive inside a location block.
 * 
 * @param loc The Location object being configured.
 * @param tokens Tokenized directive line.
 * @param i Current index in tokens; updated to skip parsed elements.
 * @throws ConfigParser::ErrorException if syntax is invalid.
 */
static void handle_location_return(Location& loc, const std::vector<std::string>& tokens, size_t& i) {
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw ConfigParser::ErrorException("Invalid return directive in location block");
    loc.setReturn(tokens[i + 1]);
    i += 2;
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
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw ConfigParser::ErrorException("Invalid alias directive in location block");
    loc.setAlias(tokens[i + 1]);
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
    while (i < tokens.size() && tokens[i] != ";") {
        loc.addCgiExtension(tokens[i++]);
    }
    if (i >= tokens.size()) throw ConfigParser::ErrorException("Missing ';' after cgi_ext directive");
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
    loc.setMaxBodySize(tokens[i + 1]);
    i += 2;
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
        handlers["return"] = handle_location_return;
        handlers["alias"] = handle_location_alias;
        handlers["cgi_path"] = handle_location_cgi_path;
        handlers["cgi_ext"] = handle_location_cgi_ext;
        handlers["client_max_body_size"] = handle_location_client_max_body_size;
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
            throw ConfigParser::ErrorException("Unknown directive: " + parameters[i]);
        }

        LocationHandler handler = it->second;
        handler(location, parameters, i);
    }

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
		handlers["server_name"] = &ServerBuilder::handle_server_name;
		handlers["root"] = &ServerBuilder::handle_root;
		handlers["client_max_body_size"] = &ServerBuilder::handle_mbs;
		handlers["autoindex"] = &ServerBuilder::handle_autoindex;
		handlers["index"] = &ServerBuilder::handle_index;
		handlers["error_page"] = &ServerBuilder::handle_error_page;
		handlers["location"] = &ServerBuilder::handle_location;
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
		if (tokens.empty()) {
			continue;
		}
		const std::string& directive = tokens[0];
		HandlerFunc handler = getHandler(directive);
		if (!handler) {
			throw ConfigParser::ErrorException("Unknown directive: '" + directive + "'");
		}
		handler(tokens, server_cfg);
	}
	return server_cfg;
}
