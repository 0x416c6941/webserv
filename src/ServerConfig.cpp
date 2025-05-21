#include "../include/ServerConfig.hpp"

ServerConfig::ServerConfig()
	: _port(0),
	  _host(""),
	  _server_name(""),
	  _root(""),
	  _client_max_body_size(0),
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

void 			ServerConfig::setPort(uint16_t port) { _port = port; }
void 			ServerConfig::setHost(const std::string& host) { _host = host; }
void 			ServerConfig::setServerName(const std::string& name) { _server_name = name; }
void 			ServerConfig::setRoot(const std::string& root) { _root = root; }
void 			ServerConfig::setIndex(const std::string& index) { _index = index; }
void 			ServerConfig::setClientMaxBodySize(uint64_t size) { _client_max_body_size = size; }
void 			ServerConfig::setAutoindex(bool mode) { _autoindex = mode; }
void 			ServerConfig::setErrorPage(int code, const std::string& path) {	this->_error_pages[code] = path;}