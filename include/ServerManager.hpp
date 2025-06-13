#pragma once
#include "Webserv.hpp"
#include "ServerConfig.hpp"
#include "ClientConnection.hpp"

/**
 * @class ServerManager
 * @brief Manages server sockets, epoll event loop, and client connections.
 *
 * The ServerManager class is responsible for:
 * - Loading server configurations.
 * - Initializing listening sockets and registering them with epoll.
 * - Running the main event loop to accept new connections and handle client events.
 * - Cleaning up resources on shutdown.
 *
 * It supports handling multiple servers, each with potentially multiple listening sockets,
 * and maps each file descriptor to its associated server configuration.
 */
class ServerManager {
private:
	std::vector<ServerConfig> 	_servers;  		//Configurations for all servers 
	int 				_epoll_fd;		//Epoll instance file descriptor
	std::map<int, ServerConfig*> 	_fd_to_server;  	//Map of socket FD to server config
	std::map<int, ClientConnection> _client_connections;	//Map of client FD to connection object

	/**
	 * @brief Accepts and registers a new client connection for a server socket.
	 * @param server_fd Listening socket file descriptor.
	 */
	void 				handleNewConnection(int server_fd);

	/**
	 * @brief Handles a ready-to-read event from a client socket.
	 * @param client_fd Client socket file descriptor.
	 */
        void 				handleClientEvent(int client_fd);


	/**
	 * @brief Registers a file descriptor with the epoll instance.
	 *
	 * @param fd File descriptor to monitor.
	 * @param events Events to watch for (e.g., EPOLLIN | EPOLLET).
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

	/**
	 * @brief Closes a client connection, cleans up its socket, and removes it from epoll.
	 * @param client_fd File descriptor of the client to close.
	 */
	void 				closeClientConnection(int client_fd);
	
	ServerManager(const ServerManager &other);
	ServerManager &operator=(const ServerManager &rhs);
public:
	ServerManager();
	~ServerManager();

	int 				getEpollFd() const;



	/**
	 * @brief Loads server configurations into the manager.
	 * @param servers Vector of ServerConfig instances to manage.
	 */
	void 				loadServers(const std::vector<ServerConfig>& servers);

	/**
	 * @brief Starts the server's main epoll-based event loop.
	 *
	 * Waits for events on registered file descriptors, and delegates to
	 * appropriate handlers (server or client).
	 */
	void 				run(); 

	/**
	 * @brief Initializes server sockets and registers them with epoll.
	 *
	 * Also sets non-blocking mode and maps sockets to their ServerConfig.
	 * @throws std::runtime_error if epoll fails or no servers are valid.
	 */
	void 				initializeSockets();

	/**
	 * @brief Cleans up all resources: sockets, epoll, and internal maps.
	 */	
	void 				cleanup();


	static void 			setupSignalHandlers();

};

 