#include "../include/ServerBuilder.hpp"

/**
 * @brief Splits a directive into individual parameters. Whitespace is used as the main delimiter,
 *        and semicolons are treated as standalone tokens.
 * 
 * @param directive The directive line (e.g., "index index.html;").
 * @return std::vector<std::string> A list of tokens, separating out ";" from other words.
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

// Utility: Validate IP (basic version)
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

void ServerBuilder::handle_host(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() != 3 || parameters[2] != ";")
		throw ConfigParser::ErrorException("Invalid syntax for host directive");

	const std::string& ip = parameters[1];
	if (!isValidIPv4(ip))
		throw ConfigParser::ErrorException("Invalid IPv4 address: " + ip);

	server_cfg.setHost(ip);
}

void ServerBuilder::handle_root(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    if (parameters.size() != 3 || parameters[2] != ";")
        throw ConfigParser::ErrorException("Invalid syntax for 'root' directive");

    server_cfg.setRoot(parameters[1]);
}

void ServerBuilder::handle_server_name(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
    if (parameters.size() != 3 || parameters[2] != ";")
        throw ConfigParser::ErrorException("Invalid syntax for 'server_name' directive");
    server_cfg.setServerName(parameters[1]);
}

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

void ServerBuilder::handle_mbs(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() < 3 || parameters.back() != ";")
		throw ConfigParser::ErrorException("Invalid syntax for client_max_body_size directive");

	try {
		uint64_t size = std::strtoull(parameters[1].c_str(), NULL, 10);
		server_cfg.setClientMaxBodySize(size);
	} catch (...) {
		throw ConfigParser::ErrorException("Invalid client_max_body_size value: " + parameters[1]);
	}
}

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


void ServerBuilder::handle_location(const std::vector<std::string>& parameters, ServerConfig& server_cfg) {
	if (parameters.size() < 3 || parameters[0] != "location" || parameters[2] != "{")
		throw ConfigParser::ErrorException("Invalid or missing URI for location block");

	Location location;
	location.setPath(parameters[1]);

	for (size_t i = 3; i < parameters.size(); ++i) {
		if (parameters[i] == "root" && i + 2 < parameters.size() && parameters[i + 2] == ";") {
			location.setRootLocation(parameters[i + 1]);
			i += 2;
		}
		else if (parameters[i] == "index") {
			i++;
			while (i < parameters.size() && parameters[i] != ";") {
				location.addIndexLocation(parameters[i++]);
			}
			if (i >= parameters.size() ) throw ConfigParser::ErrorException("Missing ';' after index directive");
		}
		else if (parameters[i] == "autoindex") {
			if (i + 1 >= parameters.size())
				throw ConfigParser::ErrorException("Missing value for autoindex directive");
			location.setAutoindex(parameters[i + 1]);
			i += 2;
		}
		else if (parameters[i] == "allow_methods") {
			location.resetMethode();
			i++;
			while (i < parameters.size() && parameters[i] != ";") {
				if (parameters[i] == "GET") location.addMethode(0);
				else if (parameters[i] == "POST") location.addMethode(1);
				else if (parameters[i] == "DELETE") location.addMethode(2);
				else if (parameters[i] == "PUT") location.addMethode(3);
				else if (parameters[i] == "HEAD") location.addMethode(4);
				else throw ConfigParser::ErrorException("Unknown method: " + parameters[i]);
				i++;
			}
			if (i >= parameters.size()) throw ConfigParser::ErrorException("Missing ';' after allow_methods directive");
		}
		else if (parameters[i] == "return" && i + 2 < parameters.size() && parameters[i + 2] == ";") {
			location.setReturn(parameters[i + 1]);
			i += 2;
		}
		else if (parameters[i] == "alias" && i + 2 < parameters.size() && parameters[i + 2] == ";") {
			location.setAlias(parameters[i + 1]);
			i += 2;
		}
		else if (parameters[i] == "cgi_path") {
			i++;
			while (i < parameters.size() && parameters[i] != ";") {
				location.setCgiPath(parameters[i++]);
			}
			if (i >= parameters.size() ) throw ConfigParser::ErrorException("Missing ';' after index directive");
		}
		else if (parameters[i] == "cgi_ext") {
			i++;
			while (i < parameters.size() && parameters[i] != ";") {
				location.addCgiExtension(parameters[i++]);
			}
			if (i >= parameters.size()) throw ConfigParser::ErrorException("Missing ';' after cgi_ext directive");
		}
		else if (parameters[i] == "client_max_body_size" && i + 2 < parameters.size() && parameters[i + 2] == ";") {
			location.setMaxBodySize(parameters[i+1]);
			i += 2;	
		}
		else if (parameters[i] == "}") {
			break;
		}
		else {
			throw ConfigParser::ErrorException("Unknown directive: " + parameters[i]);
		}
	}
	server_cfg.addLocation(location);
}



ServerConfig ServerBuilder::build(const std::vector<std::string>& directives) {
	ServerConfig server_cfg;

	for (size_t line = 0; line < directives.size(); ++line) {
		std::vector<std::string> tokens = splitParameters(directives[line]);
		if (tokens.empty())
			continue;

		const std::string& directive = tokens[0];

		if (directive == "listen") {
			handle_listen(tokens, server_cfg);
		}
		else if (directive == "host") {
			handle_host(tokens, server_cfg);
		}
		else if (directive == "server_name") {
			handle_server_name(tokens, server_cfg);
		}
		else if (directive == "root") {
			handle_root(tokens, server_cfg);
		}
		else if (directive == "client_max_body_size") {
			handle_mbs(tokens, server_cfg);
		}
		else if (directive == "autoindex") {
			handle_autoindex(tokens, server_cfg);
		}
		else if (directive == "index") {
			handle_index(tokens, server_cfg);
		}
		else if (directive == "error_page") {
			handle_error_page(tokens, server_cfg);
		}
		else if (directive == "location") {
			handle_location(tokens, server_cfg);
		}
		else {
			throw ConfigParser::ErrorException("Unknown directive: '" + directive);
		}
	}

	return server_cfg;
}

