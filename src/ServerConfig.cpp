#include "../include/ServerConfig.hpp"

// ServerConfig::ServerConfig()
// 	: _port(),
// 	  _hosts(1, "0.0.0.0"),
// 	  _server_names(),
// 	  _root("./"),
// 	  _client_max_body_size(DEFAULT_CONTENT_LENGTH),
// 	  _index(1, "index.html"),
// 	  _autoindex(false),
// 	  _listen_fds()
// {
// 	_server_addresses.clear();
// }

ServerConfig::ServerConfig()
	: _ports(),
	  _hosts(),
	  _server_names(),
	  _root(),
	  _client_max_body_size(DEFAULT_CONTENT_LENGTH),
	  _index(),
	  _autoindex(false),
	  _listen_fds()
{
	_server_addresses.clear();
}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig& other)
	: _ports(other._ports),
	  _hosts(other._hosts),
	  _server_names(other._server_names),
	  _root(other._root),
	  _client_max_body_size(other._client_max_body_size),
	  _index(other._index),
	  _autoindex(other._autoindex),
	  _error_pages(other._error_pages),
	  _locations(other._locations),
	  _server_addresses(other._server_addresses),
	  _listen_fds(other._listen_fds)
{}

ServerConfig::~ServerConfig(){}

// Getters
const std::vector<uint16_t>& 		ServerConfig::getPorts() const { return _ports; }
const std::vector<std::string>& 	ServerConfig::getHosts() const { return _hosts; }
const std::vector<std::string>& 	ServerConfig::getServerNames() const { return _server_names; }
const std::string& 			ServerConfig::getRoot() const { return _root; }
uint64_t 				ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }
const std::vector<std::string>& 	ServerConfig::getIndex() const { return _index; }
bool 					ServerConfig::getAutoindex() const { return _autoindex; }
const std::map<int, std::string>& 	ServerConfig::getErrorPages() const { return _error_pages; }
const std::vector<Location>& 		ServerConfig::getLocations() const { return _locations; }
const std::vector<sockaddr_in>& 	ServerConfig::getServerAddresses() const { return _server_addresses; }
const std::vector<int>& 		ServerConfig::getListenFds() const { return _listen_fds; }

// Setters
void 					ServerConfig::setPorts(const std::vector<uint16_t>& ports) { _ports = ports; }
void 					ServerConfig::addPort(uint16_t port) { _ports.push_back(port); }
void 					ServerConfig::setHosts(const std::vector<std::string>& hosts) { _hosts = hosts; }
void 					ServerConfig::addHost(const std::string& host) { _hosts.push_back(host); }
void 					ServerConfig::setServerNames(const std::vector<std::string>& names) { _server_names = names; }
void 					ServerConfig::addServerName(const std::string& name) { _server_names.push_back(name); }
void 					ServerConfig::setRoot(const std::string& root) { _root = root; }
void 					ServerConfig::setClientMaxBodySize(uint64_t size) { _client_max_body_size = size; }
void 					ServerConfig::setIndex(const std::vector<std::string>& index) { _index = index; }
void 					ServerConfig::addIndex(const std::string& file) { _index.push_back(file); }
void 					ServerConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void 					ServerConfig::setErrorPages(const std::map<int, std::string>& errorPages) { _error_pages = errorPages; }
void 					ServerConfig::setErrorPage(int code, const std::string& path) { _error_pages[code] = path; }
void 					ServerConfig::setLocations(const std::vector<Location>& locations) { _locations = locations; }
void 					ServerConfig::addLocation(const Location& location) { _locations.push_back(location); }
void 					ServerConfig::setServerAddresses(const std::vector<sockaddr_in>& addresses) { _server_addresses = addresses; }
void 					ServerConfig::addServerAddress(const sockaddr_in& address) { _server_addresses.push_back(address); }
void 					ServerConfig::setListenFds(const std::vector<int>& fds) { _listen_fds = fds; }
void 					ServerConfig::addListenFd(int fd) { _listen_fds.push_back(fd); }


bool 					ServerConfig::alreadyAddedHost(const std::string& host) const {
	for (std::vector<std::string>::const_iterator it = _hosts.begin(); it != _hosts.end(); ++it) {
		if (*it == host)
			return true;
	}
	return false;
}

void 					ServerConfig::resetIndex() { _index.clear(); }




// void 					ServerConfig::setClientMaxBodySize(std::string param) { 
// 	if (param.empty()) { throw ConfigParser::ErrorException("client_max_body_size cannot be empty");}

//    	char suffix = param[param.size() - 1];
//    	unsigned long multiplier = 1;

//    	if (suffix == 'K' || suffix == 'M' || suffix == 'G') {
//    	    param = param.substr(0, param.size() - 1); // remove suffix
//    	    if (suffix == 'K') multiplier = 1024UL;
//    	    else if (suffix == 'M') multiplier = 1024UL * 1024;
//    	    else if (suffix == 'G') multiplier = 1024UL * 1024 * 1024;
//    	}

//    	std::stringstream ss(param);
//    	unsigned long size;
//    	ss >> size;

//    	if (ss.fail() || !ss.eof()) {
//    	    throw ConfigParser::ErrorException("Invalid client_max_body_size: " + param + suffix);
//    	}

//    	_client_max_body_size = size * multiplier; 
// }



// void	ServerConfig::initServer(void){
// 	// Create socket
//     	_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
//     	if (_listen_fd == -1)
//         	throw ErrorException(std::string("socket creation failed: ") + strerror(errno));
	
// 	// Enable address reuse
// 	int option_value = 1;
//    	if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) == -1)
//    	{
//    	    	close(_listen_fd);
//    	    	throw ErrorException(std::string("setsockopt failed: ") + strerror(errno));
//    	}

// 	// Configure server address
// 	memset(&_server_address, 0, sizeof(_server_address));
//     	_server_address.sin_family = AF_INET;
// 		// Convert _host string to binary IP address
//     	if (inet_pton(AF_INET, _host.c_str(), &_server_address.sin_addr) != 1)
//     	{
//         	close(_listen_fd);
//         	throw ErrorException("Invalid IP address: " + _host);
//     	}
//     	_server_address.sin_port = htons(_port);

// 	// Bind socket
//     	if (bind(_listen_fd, (struct sockaddr *)&_server_address, sizeof(_server_address)) == -1)
//     	{
//     	    	close(_listen_fd);
//     	    	throw ErrorException(std::string("bind failed: ") + strerror(errno));
//     	}

//    	// Start listening
//    	if (listen(_listen_fd, SOMAXCONN) == -1)
//    	{
//    	    close(_listen_fd);
//    	    throw ErrorException(std::string("listen failed: ") + strerror(errno));
//    	}
//     	std::ostringstream oss;
// 	oss << "webserv: Server " << _server_name << " is listening on port " << _port;
// 	print_log(oss.str());

// }

// void ServerConfig::cleanupSocket()
// {
//     if (_listen_fd != -1)
//         close(_listen_fd);
// }
