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


	/**
 	* @brief Registers a file descriptor with the epoll instance.
 	*
 	* @param fd        File descriptor to monitor.
 	* @param events    Events to watch for (e.g., EPOLLIN | EPOLLET).
 	* @return true if successful, false if epoll_ctl failed.
 	*/
	bool 				addFdToEpoll(int fd, uint32_t events);

	/**
	* @brief Removes a file descriptor from the epoll instance.
	*
	* @param fd File descriptor to remove.
	* @return true if successful, false otherwise.
	*/
	bool 				removeFdFromEpoll(int fd);

	void 				closeClientConnection(int client_fd);
	
public:
	ServerManager();
	~ServerManager();
	
	void 				loadServers(const std::vector<ServerConfig>& servers);
	void 				run(); 

	// Helpers
	void 				initializeSockets();
	void 				cleanup();
	int 				getEpollFd() const;


	static void 			setupSignalHandlers();

};

 