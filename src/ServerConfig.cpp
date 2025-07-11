#include "../include/ServerConfig.hpp"

ServerConfig::ServerConfig()
	: _listen_endpoints(),
	  _ports(),
	  _hosts(),
	//   _server_names(),
	  _root(),
	  _client_max_body_size(DEFAULT_CONTENT_LENGTH),
	  _index(),
	  _autoindex(false),
	  _listen_fds(),
	  _large_client_header_buffers(DEFAULT_LARGE_CLIENT_HEADER_BUFFERS, DEFAULT_LARGE_CLIENT_HEADER_BUFFER_SIZE)
{
	_server_addresses.clear();
	_listen_fds.clear();
}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig& other)
	: _listen_endpoints(other._listen_endpoints),
	  _ports(other._ports),
	  _hosts(other._hosts),
	//   _server_names(other._server_names),
	  _root(other._root),
	  _client_max_body_size(other._client_max_body_size),
	  _index(other._index),
	  _autoindex(other._autoindex),
	  _error_pages(other._error_pages),
	  _locations(other._locations),
	  _server_addresses(other._server_addresses),
	  _listen_fds(other._listen_fds),
	  _large_client_header_buffers(other._large_client_header_buffers)

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
// const std::vector<std::string>& 	ServerConfig::getServerNames() const { return _server_names; }
const std::string& 			ServerConfig::getRoot() const { return _root; }
uint64_t 				ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }
const std::vector<std::string>& 	ServerConfig::getIndex() const { return _index; }
bool 					ServerConfig::getAutoindex() const { return _autoindex; }
const std::map<int, std::string>& 	ServerConfig::getErrorPages() const { return _error_pages; }
const std::vector<Location>& 		ServerConfig::getLocations() const { return _locations; }
const std::vector<sockaddr_in>& 	ServerConfig::getServerAddresses() const { return _server_addresses; }
const std::vector<int>& 		ServerConfig::getListenFds() const { return _listen_fds; }
std::pair<uint32_t, uint64_t> 		ServerConfig::getLargeClientHeaderBuffers() const { return _large_client_header_buffers;}
// Optional: Individual getters
uint32_t ServerConfig::getLargeClientHeaderBufferCount() const {
	return _large_client_header_buffers.first;
}

uint64_t ServerConfig::getLargeClientHeaderBufferSize() const {
	return _large_client_header_buffers.second;
}

uint64_t ServerConfig::getLargeClientHeaderTotalBytes() const {
	return _large_client_header_buffers.second * _large_client_header_buffers.first;
}

// Setters
void ServerConfig::addListenEndpoint(const std::pair<std::string, uint16_t>& endpoint) {
	_listen_endpoints.push_back(endpoint);
}
void 					ServerConfig::setPorts(const std::vector<uint16_t>& ports) { _ports = ports; }
void 					ServerConfig::addPort(uint16_t port) { _ports.push_back(port); }
void 					ServerConfig::setHosts(const std::vector<std::string>& hosts) { _hosts = hosts; }
void 					ServerConfig::addHost(const std::string& host) { _hosts.push_back(host); }
// void 					ServerConfig::setServerNames(const std::vector<std::string>& names) { _server_names = names; }
// void 					ServerConfig::addServerName(const std::string& name) { _server_names.push_back(name); }
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
void 					ServerConfig::setLargeClientHeaderBuffers(uint32_t count, uint64_t sizeInBytes) {
	_large_client_header_buffers.first = count;
	_large_client_header_buffers.second = sizeInBytes;
}


bool 					ServerConfig::alreadyAddedHost(const std::string& host) const {
	for (std::vector<std::string>::const_iterator it = _hosts.begin(); it != _hosts.end(); ++it) {
		if (*it == host)
			return true;
	}
	return false;
}

void 					ServerConfig::resetIndex() { _index.clear(); }

/**
 * @brief Sets default server configuration values if not explicitly provided.
 *
 * Assigns defaults for root directory, index files, and error pages.
 * Ensures at least one listening port is configured, and sets a default host
 * if none is specified. Throws `ErrorException` if no listen directive is present.
 */
void ServerConfig::setDefaultsIfEmpty() {
	// Default root
	if (_root.empty()) {
		_root = "./";
	}

	// Default index
	if (_index.empty()) {
		_index.push_back("index.html");
	}

	if (_ports.empty() && _listen_endpoints.empty()) {
		throw ServerConfig::ErrorException("No 'listen' directive provided: server must specify at least one port.");
	}

	// Default host (if only port is defined and no endpoint provided)
	if (_hosts.empty() && !_ports.empty() && _listen_endpoints.empty()) {
		_hosts.push_back("0.0.0.0");
	}

	if (_client_max_body_size == 0) {
		_client_max_body_size = DEFAULT_CONTENT_LENGTH; // Default to 1MB
	}
}

/**
 * @brief Validates that all configured listen endpoints are unique.
 *
 * Ensures there are no duplicate entries in `_listen_endpoints` or in the
 * combinations of `_hosts` and `_ports`. Throws `ErrorException` if duplicates
 * are found.
 */
void ServerConfig::validateListenEndpoint(void)
{
	std::set<std::pair<std::string, uint16_t> > unique_endpoints;

	// Validate explicitly defined endpoints
	for (size_t i = 0; i < _listen_endpoints.size(); ++i) {
		const std::pair<std::string, uint16_t>& ep = _listen_endpoints[i];
		if (!unique_endpoints.insert(ep).second) {
			std::ostringstream oss;
			oss << "Duplicate listen endpoint: " << ep.first << ":" << ep.second;
			throw ErrorException(oss.str());
		}
	}

	// Validate host+port combinations
	for (size_t i = 0; i < _hosts.size(); ++i) {
		for (size_t j = 0; j < _ports.size(); ++j) {
			std::pair<std::string, uint16_t> legacy_pair = std::make_pair(_hosts[i], _ports[j]);
			if (!unique_endpoints.insert(legacy_pair).second) {
				std::ostringstream oss;
				oss << "Duplicate listen host+port combination: " << _hosts[i] << ":" << _ports[j];
				throw ErrorException(oss.str());
			}
		}
	}
}

/**
 * @brief Creates, binds, and listens on a TCP socket for the given host and port.
 *
 * Initializes a non-blocking socket, sets socket options, binds it to the
 * specified IPv4 address and port, and starts listening for incoming connections.
 *
 * @param host The IP address to bind the socket to (as a string).
 * @param port The port number to bind the socket to.
 * @param out_addr Reference to a sockaddr_in structure that will be filled with the bound address.
 * @return int File descriptor of the created socket on success, or -1 on failure.
 */
int ServerConfig::createListeningSocket(const std::string& host, uint16_t port, sockaddr_in& out_addr) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;

	// Allow socket reuse to avoid "Address already in use" on quick restart
	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		close(fd);
		return -1;
	}

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

	return fd;
}

void ServerConfig::validateRoot() {
        if (_root.empty()) {
                throw ErrorException("Root directory is not set. Please specify a valid root path.");
        }

        struct stat sb;
        if (stat(_root.c_str(), &sb) != 0) {
                throw ErrorException("Root directory does not exist or is not accessible: " + _root);
        }

        if (!S_ISDIR(sb.st_mode)) {
                throw ErrorException("Root path is not a directory: " + _root);
        }

	if (access(_root.c_str(), R_OK | X_OK) != 0) {
        	throw ErrorException("Insufficient permissions to access root directory: " + _root);
	}
}


/**
 * @brief Initializes and binds server sockets to configured host-port pairs.
 *
 * Sets default values for server var if missing, validates the
 * `_listen_endpoints`, then creates listening sockets for all specified
 * endpoints and host-port combinations. On success, stores socket descriptors
 * and addresses. Throws if binding any socket fails.
 */
void ServerConfig::initServerSocket() {
	setDefaultsIfEmpty();
	validateListenEndpoint();
	validateRoot();
	print_log("", "Initializing server sockets...", "");

	// 1. Explicit host:port endpoints
	for (size_t i = 0; i < _listen_endpoints.size(); ++i) {
		const std::string& host = _listen_endpoints[i].first;
		uint16_t port = _listen_endpoints[i].second;

		sockaddr_in addr;
		int fd = createListeningSocket(host, port, addr);
		if (fd < 0) {
			std::string err_msg = "Failed to bind: " + host + ":" + to_string(port) + ": " + std::strerror(errno);
			throw std::runtime_error(err_msg);
		}

		std::string endpoint = host + ":" + to_string(port);
		print_log("Successfully listening on ", endpoint, " (fd: " + to_string(fd) + ")");
		_server_addresses.push_back(addr);
		_listen_fds.push_back(fd);
	}

	// 2. All host x port combinations
	for (size_t i = 0; i < _hosts.size(); ++i) {
		for (size_t j = 0; j < _ports.size(); ++j) {
			const std::string& host = _hosts[i];
			uint16_t port = _ports[j];

			sockaddr_in addr;
			int fd = createListeningSocket(host, port, addr);
			if (fd < 0) {
				std::string err_msg = "Failed to bind: " + host + ":" + to_string(port) + ": " + std::strerror(errno);
				throw std::runtime_error(err_msg);
			}

			std::string endpoint = host + ":" + to_string(port);
			print_log("Successfully listening on ", endpoint, " (fd: " + to_string(fd) + ")");
			_server_addresses.push_back(addr);
			_listen_fds.push_back(fd);
		}
	}
}

/**
 * @brief Closes all listening sockets and clears related server data.
 *
 * Attempts to close each socket in `_listen_fds`, logging success or failure,
 * then clears `_listen_fds` and `_server_addresses`. Throws an exception if
 * any socket fails to close.
 */
void ServerConfig::cleanupSocket() {
	for (size_t i = 0; i < _listen_fds.size(); ++i) {
		int fd = _listen_fds[i];
		if (close(fd) == 0) {
			print_log("Closed listening socket", " (fd: " + to_string(fd) + ")", "");
		} else {
			print_warning("Failed to close socket", " (fd: " + to_string(fd) + "): " + std::strerror(errno), "");
			throw std::runtime_error("Failed to close socket: " + std::string(std::strerror(errno)));
		}
	}
	_listen_fds.clear();
	_server_addresses.clear();
}
