#include "../include/Webserv.hpp"
#include "../include/ConfigParser.hpp"

void printServerConfig(const ServerConfig& config)
{
	std::cout << "====== Server Configuration ======" << std::endl;

	// Server names
	const std::vector<std::string>& names = config.getServerNames();
	std::cout << "Server Names:";
	if (names.empty()) std::cout << " (none)";
	for (size_t i = 0; i < names.size(); ++i)
		std::cout << " " << names[i];
	std::cout << std::endl;

	// Hosts
	const std::vector<std::string>& hosts = config.getHosts();
	std::cout << "Hosts:";
	if (hosts.empty()) std::cout << " (none)";
	for (size_t i = 0; i < hosts.size(); ++i)
		std::cout << " " << hosts[i];
	std::cout << std::endl;

	// Ports
	const std::vector<uint16_t>& ports = config.getPorts();
	std::cout << "Ports:";
	if (ports.empty()) std::cout << " (none)";
	for (size_t i = 0; i < ports.size(); ++i)
		std::cout << " " << ports[i];
	std::cout << std::endl;

	// Root
	std::cout << "Root: " << config.getRoot() << std::endl;

	// Index files
	const std::vector<std::string>& indexes = config.getIndex();
	std::cout << "Index Files:";
	if (indexes.empty()) std::cout << " (none)";
	for (size_t i = 0; i < indexes.size(); ++i)
		std::cout << " " << indexes[i];
	std::cout << std::endl;

	// Autoindex
	std::cout << "Autoindex: " << (config.getAutoindex() ? "on" : "off") << std::endl;

	// Max body size
	std::cout << "Max Client Body Size: " << config.getClientMaxBodySize() << " bytes" << std::endl;

	// Error pages
	const std::map<int, std::string>& errors = config.getErrorPages();
	std::cout << "Error Pages: " << errors.size() << std::endl;
	for (std::map<int, std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it)
		std::cout << "  " << it->first << " => " << it->second << std::endl;

	// Locations
	const std::vector<Location>& locations = config.getLocations();
	std::cout << "Locations: " << locations.size() << std::endl;
	for (size_t i = 0; i < locations.size(); ++i) {
		std::cout << "--- Location #" << (i + 1) << " ---" << std::endl;
		locations[i].printDebug(); // assumes you implemented printDebug() in Location
	}

	// Bound addresses
	const std::vector<sockaddr_in>& addrs = config.getServerAddresses();
	std::cout << "Bound Addresses: " << addrs.size() << std::endl;
	for (size_t i = 0; i < addrs.size(); ++i) {
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addrs[i].sin_addr), ip, INET_ADDRSTRLEN);
		std::cout << "  " << ip << ":" << ntohs(addrs[i].sin_port) << std::endl;
	}

	// Listen file descriptors
	const std::vector<int>& fds = config.getListenFds();
	std::cout << "Listen FDs:";
	if (fds.empty()) std::cout << " (none)";
	for (size_t i = 0; i < fds.size(); ++i)
		std::cout << " " << fds[i];
	std::cout << std::endl;

	std::cout << "==================================" << std::endl;
}
