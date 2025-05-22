#include "../include/ConfigParser.hpp"


ConfigParser::ConfigParser(const std::string& content)
	: _rawContent(content) {}

ConfigParser::~ConfigParser() {}


void 	ConfigParser::cleanLinesInPlace(std::string& block) {
	std::istringstream iss(block);
	std::ostringstream cleaned;
	std::string line;

	while (std::getline(iss, line)) {
		line = trim(line);
		if (!line.empty())
			cleaned << line << "\n";
	}

	block = cleaned.str();
}


/**
 * @brief Remove all comments in the config file from # until a new line.
 * @warning New line isn't deleted to save the proper indentation
 * @param content - readed config file 
 */
void 	ConfigParser::removeComments(std::string &content) {
	size_t pos = content.find('#');
	while (pos != std::string::npos) {
		size_t n_pos = content.find('\n', pos);
		if (n_pos == std::string::npos) {
			content.erase(pos);
			break;
		}
		content.erase(pos, n_pos - pos);
		pos = content.find('#');
	}
}


/* finding a server begin and return the index of { start of server */
size_t ConfigParser::findStartServer(size_t start, const std::string &content)
{
    size_t i = start;

    // 1. Skip leading whitespace and validate nothing invalid comes before "server"
    while (i < content.size() && isspace(content[i]))
        ++i;

    // 2. Check if "server" starts here
    if (i + 6 > content.size() || content.compare(i, 6, "server") != 0)
        throw ErrorException("Expected 'server' directive");

    i += 6;

    // 3. Skip whitespace after "server"
    while (i < content.size() && isspace(content[i]))
        ++i;

    // 4. Expect '{' to follow
    if (i < content.size() && content[i] == '{')
        return i;

    throw ErrorException("Expected '{' after 'server' directive");
}

/* Finds the matching closing '}' for a server block starting at '{' */
size_t ConfigParser::findEndServer(size_t start, const std::string &content)
{
    if (start >= content.size() || content[start] != '{')
        throw ErrorException("Expected '{' at the start of server block");

    size_t depth = 1;
    for (size_t i = start + 1; i < content.size(); ++i)
    {
        if (content[i] == '{')
            ++depth;
        else if (content[i] == '}')
        {
            --depth;
            if (depth == 0)
                return i; // found the matching closing brace
        }
    }

    // If we reach here, we never found the matching '}'
    throw ErrorException("Unmatched '{' in server block");
}



void ConfigParser::splitIntoServerBlocks(const std::string &content) {
	size_t start = 0;

	if (content.find("server", 0) == std::string::npos)
		throw ErrorException("No 'server' block found");

	while (start < content.length()) {
		start = content.find("server", start);
		if (start == std::string::npos)
			break;
		size_t braceStart = findStartServer(start, content);
		size_t braceEnd = findEndServer(braceStart, content);
		if (braceEnd <= braceStart)
			throw ErrorException("Malformed server block");

		std::string block = content.substr(braceStart + 1, braceEnd - braceStart - 1);
		cleanLinesInPlace(block);
		_serverBlocks.push_back(block);
		start = braceEnd + 1;
	}
}



std::vector<std::string> ConfigParser::splitDirectives(const std::string &block) {
	std::vector<std::string> directives;
	std::string current;
	size_t depth = 0;

	for (size_t i = 0; i < block.size(); ++i) {
		char c = block[i];
		current += c;

		if (c == '{') ++depth;
		else if (c == '}') {
			if (depth == 0)
				throw ErrorException("Unmatched '}' in directive block");
			--depth;
		}

		if ((c == ';' && depth == 0) || (c == '}' && depth == 0)) {
			directives.push_back(trim(current));
			current.clear();
		}
	}
	if (!trim(current).empty())
		throw ErrorException("Unterminated or malformed directive: " + current);

	return directives;
}



void ConfigParser::parse() {
	removeComments(_rawContent);
	splitIntoServerBlocks(_rawContent);
	for (size_t i = 0; i < _serverBlocks.size(); ++i) {
		std::vector<std::string> directives = splitDirectives(_serverBlocks[i]);
		if (DEBUG)
		{
			std::cout << "\nParsed Directives for Server Block #" << i << ":\n";
			for (size_t j = 0; j < directives.size(); ++j) {
				std::cout << "  [" << j << "] " << directives[j] << std::endl;
			}
		}
		ServerConfig server = ServerBuilder::build(directives);
		_servers.push_back(server);
	}
}




const std::vector<std::string>& ConfigParser::getServerBlocks() const {
	return _serverBlocks;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {
	return _servers;
}

void ConfigParser::print()
{
	std::cout << "------------- Config -------------" << std::endl;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::cout << "Server #" << i + 1 << std::endl;
		std::cout << "Server name: " << _servers[i].getServerName() << std::endl;
		std::cout << "Host: " << _servers[i].getHost() << std::endl;
		std::cout << "Root: " << _servers[i].getRoot() << std::endl;
		std::cout << "Index: " << _servers[i].getIndex() << std::endl;
		std::cout << "Port: " << _servers[i].getPort() << std::endl;
		std::cout << "Max BSize: " << _servers[i].getClientMaxBodySize() << std::endl;
		std::cout << "Error pages: " << _servers[i].getErrorPages().size() << std::endl;
		std::map<int, std::string>::const_iterator it = _servers[i].getErrorPages().begin();
		while (it != _servers[i].getErrorPages().end())
		{
			std::cout << it->first << " - " << it->second << std::endl;
			++it;
		}
		const std::vector<Location>& locations = _servers[i].getLocations();
		std::cout << "Locations: " << locations.size() << std::endl;
		for (size_t j = 0; j < locations.size(); ++j)
		{
			std::cout << "--- Location #" << j + 1 << " ---" << std::endl;
			locations[j].printDebug();
		}
		std::cout << "-----------------------------" << std::endl;

			
	
	}
}