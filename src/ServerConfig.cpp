#include "../include/ServerConfig.hpp"

ServerConfig::ServerConfig()
	: _port(0),
	  _host(""),
	  _server_name(""),
	  _root(""),
	  _client_max_body_size(MAX_CONTENT_LENGTH),
	  _index(""),
	  _autoindex(false),
	  _listen_fd(-1)
{
	std::memset(&_server_address, 0, sizeof(_server_address));
}

ServerConfig::~ServerConfig(){}

uint16_t 				ServerConfig::getPort() const { return _port; }
const std::string& 			ServerConfig::getHost() const { return _host; }
const std::string& 			ServerConfig::getServerName() const { return _server_name; }
const std::string& 			ServerConfig::getRoot() const { return _root; }
const std::string& 			ServerConfig::getIndex() const { return _index; }
uint64_t 				ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }
bool 					ServerConfig::getAutoindex() const { return _autoindex; }
const std::map<int, std::string>& 	ServerConfig::getErrorPages() const {return _error_pages;}
const std::vector<Location>		&ServerConfig::getLocations() const {return _locations;}

void 			ServerConfig::setPort(uint16_t port) { _port = port; }
void 			ServerConfig::setHost(const std::string& host) { _host = host; }
void 			ServerConfig::setServerName(const std::string& name) { _server_name = name; }
void 			ServerConfig::setRoot(const std::string& root) { _root = root; }
void 			ServerConfig::setIndex(const std::string& index) { _index = index; }

void 			ServerConfig::setAutoindex(bool mode) { _autoindex = mode; }
void 			ServerConfig::setErrorPage(int code, const std::string& path) {	this->_error_pages[code] = path;}
void 			ServerConfig::addLocation(const Location& loc_section){_locations.push_back(loc_section);}

void 			ServerConfig::setClientMaxBodySize(uint64_t size) { _client_max_body_size = size; }
void 			ServerConfig::setClientMaxBodySize(std::string param) { 
	if (param.empty()) { throw ConfigParser::ErrorException("client_max_body_size cannot be empty");}

   	char suffix = param[param.size() - 1];
   	unsigned long multiplier = 1;

   	if (suffix == 'K' || suffix == 'M' || suffix == 'G') {
   	    param = param.substr(0, param.size() - 1); // remove suffix
   	    if (suffix == 'K') multiplier = 1024UL;
   	    else if (suffix == 'M') multiplier = 1024UL * 1024;
   	    else if (suffix == 'G') multiplier = 1024UL * 1024 * 1024;
   	}

   	std::stringstream ss(param);
   	unsigned long size;
   	ss >> size;

   	if (ss.fail() || !ss.eof()) {
   	    throw ConfigParser::ErrorException("Invalid client_max_body_size: " + param + suffix);
   	}

   	_client_max_body_size = size * multiplier; 
}
