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
	: _listen_endpoints(),
	  _ports(),
	  _hosts(),
	  _server_names(),
	  _root(),
	  _client_max_body_size(DEFAULT_CONTENT_LENGTH),
	  _index(),
	  _autoindex(false),
	  _listen_fds()
{
	_server_addresses.clear();
	_listen_fds.clear();
}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig& other)
	: _listen_endpoints(other._listen_endpoints),
	  _ports(other._ports),
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

ServerConfig::~ServerConfig(){
	cleanupSocket();
}

// Getters
const std::vector<std::pair<std::string, uint16_t> >& ServerConfig::getListenEndpoints() const {
	return _listen_endpoints;
}

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
void ServerConfig::addListenEndpoint(const std::pair<std::string, uint16_t>& endpoint) {
	_listen_endpoints.push_back(endpoint);
}
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

int ServerConfig::createListeningSocket(const std::string& host, uint16_t port, sockaddr_in& out_addr) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	std::memset(&out_addr, 0, sizeof(out_addr));
	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, host.c_str(), &out_addr.sin_addr) <= 0) {
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr*)&out_addr, sizeof(out_addr)) < 0) {
		close(fd);
		return -1;
	}

	if (listen(fd, SOMAXCONN) < 0) {
		close(fd);
		return -1;
	}

	// Optional for epoll:
	// fcntl(fd, F_SETFL, O_NONBLOCK);

	return fd;
}

void ServerConfig::initServer() {
	if (_hosts.empty()) _hosts.push_back("0.0.0.0");
	if (_ports.empty()) _ports.push_back(8080);

	std::set<std::string> boundPairs;

	for (size_t i = 0; i < _hosts.size(); ++i) {
		for (size_t j = 0; j < _ports.size(); ++j) {
			std::string key = _hosts[i] + ":" + std::string(1, ':') + std::string("0") + std::string(1, '0'); // C++98 doesn't have std::to_string
			char buf[6];
			sprintf(buf, "%u", _ports[j]);
			key = _hosts[i] + ":" + std::string(buf);

			if (boundPairs.find(key) != boundPairs.end())
				continue;

			sockaddr_in addr;
			int fd = createListeningSocket(_hosts[i], _ports[j], addr);

			if (fd < 0)
				continue; // Or log error

			_server_addresses.push_back(addr);
			_listen_fds.push_back(fd);
			boundPairs.insert(key);
		}
	}
}


void ServerConfig::cleanupSocket(){
	for (size_t i = 0; i < _listen_fds.size(); ++i) {
		close(_listen_fds[i]);
	}
	_listen_fds.clear();
}
