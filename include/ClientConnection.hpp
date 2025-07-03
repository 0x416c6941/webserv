#pragma once

#include "Webserv.hpp"
#include <string>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

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
	bool 		  	_request_header_found;
	bool                    _request_error;
	bool		    	_msg_sent; // Indicates if the request is fully sent
	size_t 			_bytes_sent;
	// TCP is a streaming oriented protocol, we therefore
	// need a buffer for the request until it's fully parsed.
	std::string		_request_buffer;
	// Since `_request_buffer` will be dynamically appended
	// and cleared all the time (during request parsing),
	// we need to check how many bytes of each header and body buffers
	// we have already exhausted.
	size_t			_header_buffer_bytes_exhausted;
	size_t			_body_buffer_bytes_exhausted;

	/**
	* @brief Returns the length of the HTTP header in the request buffer.
	*
	* Finds the end-of-header delimiter ("\r\n\r\n") and returns its position
	* plus the delimiter length. Returns -1 if the header is incomplete.
	*
	* @param requestBuffer Raw HTTP request data.
	* @return int Length of the header including the delimiter, or -1 if not found.
	*/
	int 	getHttpHeaderLength(const std::string& requestBuffer);

public:
	HTTPRequest             _request;
	HTTPResponse            _response;
	ClientConnection();
	ClientConnection(int fd);
	~ClientConnection();
	ClientConnection &operator=(const ClientConnection &rhs); //do we need this? need to update
	ClientConnection(const ClientConnection &other);	//do we need this? need to update

	// Accessors
	int                     getSocket() const;
	const struct sockaddr_in& getAddress() const;
	time_t                  getLastTime() const;
	ServerConfig*           getServer() const;
	bool			getRequestIsComplete() const;
	bool			getRequestError() const;
	bool			getResponseReady() const;
	bool			getMsgSent() const;
	size_t			getRequestHeaderBufferBytesExhaustion() const;
	size_t			getRequestBodyBufferBytesExhaustion() const;
	HTTPRequest&          	getRequest();

	// Mutators
	void                    setSocket(int socket);
	void                    setAddress(const struct sockaddr_in &addr);
	void                    setServer(ServerConfig &server);
	void                    updateTime();

	// Logic
	bool                    handleReadEvent();
	bool		    	handleWriteEvent();
	int                  	parseReadEvent(std::string &buffer);
	void                    closeConnection();
	void 			reset();

	//Debug
	void 			printDebugRequestParse();

};
