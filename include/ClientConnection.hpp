#pragma once
#include "Webserv.hpp"
// #include "HttpRequest.hpp"
// #include "Response.hpp"

/**
 * @brief Represents a single client connection.
 *
 * Stores socket descriptor, client address, associated server configuration,
 * and last activity timestamp. Handles reading from the socket and managing
 * connection state.
 */
class ClientConnection {
private:
	int                     _client_socket;
	struct sockaddr_in      _client_address;
	ServerConfig*           _server;
	time_t                  _last_msg_time;

	// HttpRequest             _request;
	// Response                _response;

        
        public:
	ClientConnection();
	ClientConnection(int fd);
	~ClientConnection();
	ClientConnection(const ClientConnection &other);
	ClientConnection &operator=(const ClientConnection &rhs);

	// Accessors
	int                     getSocket() const;
	const struct sockaddr_in& getAddress() const;
	time_t                  getLastTime() const;
	ServerConfig*           getServer() const;

	// Mutators
	void                    setSocket(int socket);
	void                    setAddress(const struct sockaddr_in &addr);
	void                    setServer(ServerConfig &server);
	void                    updateTime();

	// Logic
	bool                    handleRead();
	void                    closeConnection();
};
