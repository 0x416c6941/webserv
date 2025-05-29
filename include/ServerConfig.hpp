#pragma once
#include "Webserv.hpp"
#include "Location.hpp"

class Location;

/**
 * @class ServerConfig
 * @brief Represents the configuration for a single server instance.
 *
 * This class encapsulates server-wide configuration details such as
 * port, host, root directory, index file, client body size limits,
 * error pages, and route-specific location blocks.
 */
class ServerConfig
{
private:
	std::vector<uint16_t> 		_ports;			// Port number
	std::vector<std::string>	_hosts;			// IP address (IPv4)
	std::vector<std::string>	_server_names;		// Server name / domain
	std::string			_root;			// Root directory path
	uint64_t			_client_max_body_size;	// Max client body size (bytes)
	std::vector<std::string>	_index;			// Default index file
	bool				_autoindex;		// Directory listing toggle
	std::map<int, std::string>	_error_pages;		// Error code to custom page mapping
	std::vector<Location>		_locations;		// List of route-specific configurations
	std::vector<sockaddr_in>	_server_addresses;	// Full IPv4 socket address struct
	std::vector<int>		_listen_fds;		// Socket file descriptor
	
public:
	ServerConfig();
	ServerConfig(const ServerConfig& other);
	~ServerConfig();

	// Getters
	const std::vector<uint16_t>& 	getPorts() const;
	const std::vector<std::string>& getHosts() const;
	const std::vector<std::string>& getServerNames() const;
	const std::string& 		getRoot() const;
	uint64_t 			getClientMaxBodySize() const;
	const std::vector<std::string>& getIndex() const;
	bool 				getAutoindex() const;
	const std::map<int, std::string>&getErrorPages() const;
	const std::vector<Location>& 	getLocations() const;
	const std::vector<sockaddr_in>& getServerAddresses() const;
	const std::vector<int>& 	getListenFds() const;



	// Setters
	void 				setPorts(const std::vector<uint16_t>& ports);
	void 				addPort(uint16_t port);
	void 				setHosts(const std::vector<std::string>& hosts);
	void 				addHost(const std::string& host);
	void 				setServerNames(const std::vector<std::string>& names);
	void 				addServerName(const std::string& name);
	void 				setRoot(const std::string& root);
	void 				setClientMaxBodySize(uint64_t size);
	void 				setIndex(const std::vector<std::string>& index);
	void 				addIndex(const std::string& file);
	void 				setAutoindex(bool autoindex);
	void 				setErrorPages(const std::map<int, std::string>& errorPages);
	void 				setErrorPage(int code, const std::string& path);
	void 				setLocations(const std::vector<Location>& locations);
	void 				addLocation(const Location& location);
	void 				setServerAddresses(const std::vector<sockaddr_in>& addresses);
	void 				addServerAddress(const sockaddr_in& address);
	void 				setListenFds(const std::vector<int>& fds);
	void 				addListenFd(int fd);

	bool 				alreadyAddedHost(const std::string& host) const;
	void 				resetIndex(void);
	

	void				initServer(void);
	void 				cleanupSocket(void);
	
	public:
		class ErrorException : public std::exception
		{
			private:
				std::string _message;
			public:
				ErrorException(std::string message) throw() {
					_message = "SERVER INIT ERROR: " + message;
				}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};

