#pragma once
#include "Webserv.hpp"
#include "ServerConfig.hpp"
#include "ClientConnection.hpp"

class ClientConnection;
class ServerManager {
private:
	std::vector<ServerConfig> 	_servers;
	int 				_epoll_fd;
	std::map<int, ServerConfig*> 	_fd_to_server;
	std::map<int, ClientConnection> _client_connections;

	// Private methods
	void 				handleNewConnection(int server_fd);
        void 				handleClientEvent(int client_fd);
	
	public:
	ServerManager();
	~ServerManager();
	
	void 				loadServers(const std::vector<ServerConfig>& servers);
	void 				run(); 

	// Helpers
	void 				initializeSockets();
	void 				cleanup();
	int 				getEpollFd() const;
};

 