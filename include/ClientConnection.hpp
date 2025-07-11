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
	 * Determines the max body size of files sent to us with "POST" method
	 * depending on \p target.
	 * @throw	domain_error	Saving a received file at \p path
	 * 				is forbidden.
	 * @param	target	Request target parsed in request header.
	 * @return	Max body size of received file.
	 */
	size_t			getMaxBodySize(const std::string &target) const;

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
	/**r.reset();
	 * Reads a limited (by buffer size) part of information
	 * sent to us by the client and tries to parse it.
	 * @return	true, if some information was successfully read and parsed;
	 * 		false, if an error occurred or the client closed the connection.
	 */
	bool                    handleReadEvent();

	/**
	 * Reads and processes request information from \p buffer.
	 * @throw	range_error	Request is already complete.
	 * @param	buffer	Information sent to us by the client,
	 * 			read in `handleReadEvent()`.
	 * @return	0, if everything went alright
	 * 			(this doesn't mean that request is complete,
	 * 			check request completeness with
	 * 			`getRequestIsComplete()` method);
	 * 		HTTP error code instead.
	 */
	int                  	parseReadEvent(std::string &buffer);

	/**
	 * Determines which Location from `_server` corresponds to \p target.
	 * @throw	out_of_range	Location with such target
	 * 				isn't defined.
	 * @param	target	Request target parsed in request header.
	 * @return	Determined Location.
	 */
	const Location		&determineLocation(const std::string &target) const;

	// TODO: Should we send the data with send() in limited packets,
	// e.g. 2 KBs?
	// Doing so could potentially prevent problems such as
	// big files not being able to be sent completely over the network.
	bool		    	handleWriteEvent();

	void                    closeConnection();
	void 			reset();

	//Debug
	void 			printDebugRequestParse();

};
