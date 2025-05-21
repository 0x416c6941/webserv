#pragma once
#include "Webserv.hpp"

// class Location; 

class ServerConfig
{
private:
	uint16_t			_port;			// Port number
	std::string			_host;			// IP address (IPv4)
	std::string			_server_name;		// Server name / domain
	std::string			_root;			// Root directory path
	uint64_t			_client_max_body_size;	// Max client body size (bytes)
	std::string			_index;			// Default index file
	bool				_autoindex;		// Directory listing toggle
	std::map<int, std::string>	_error_pages;		// Error code to custom page mapping
	// std::vector<Location>		_locations;		// List of route-specific configurations
	sockaddr_in			_server_address;	// Full IPv4 socket address struct
	int				_listen_fd;		// Socket file descriptor
	
public:
	ServerConfig();
	~ServerConfig();
	// Getters
	uint16_t 			getPort() const;
	const std::string& 		getServerName() const;
	const std::string& 		getRoot() const;
	const std::string& 		getIndex() const;
	uint64_t 			getClientMaxBodySize() const;
	bool 				getAutoindex() const;


	// Setters
	void 				setPort(uint16_t port);
	void 				setServerName(const std::string& name);
	void 				setRoot(const std::string& root);
	void 				setIndex(const std::string& index);
	void 				setClientMaxBodySize(uint64_t size);
	void 				setAutoindex(bool mode);
};

