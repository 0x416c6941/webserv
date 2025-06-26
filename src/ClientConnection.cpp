#include "../include/ClientConnection.hpp"

ClientConnection::ClientConnection(int fd)
	: _client_socket(fd),
	_client_address(),
	_server(NULL),
	_last_msg_time(std::time(NULL)),
	// _request(),
	_request_header_found(false),
	_request_error(false),
	_msg_sent(false),
	_bytes_sent(0),
	_request_buffer()
	// _response()
{
    std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection()
	: _client_socket(-1),
	_client_address(),
	_server(NULL),
	_last_msg_time(std::time(NULL)),
	// _request(),
	_request_header_found(false),
	_request_error(false),
	_msg_sent(false),
	_bytes_sent(0),
	_request_buffer()
	// _response()
{
    std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection(const ClientConnection& other)
	: _client_socket(other._client_socket),
	  _client_address(other._client_address),
	  _server(other._server),  // shallow copy (non-owning)
	  _last_msg_time(other._last_msg_time),
	//   _request(other._request),
	  _request_header_found(other._request_header_found),
	  _request_error(other._request_error),
	  _msg_sent(other._msg_sent),
	  _bytes_sent(other._bytes_sent),
	  _request_buffer(other._request_buffer)
	//   _response(other._response)
{}

ClientConnection& ClientConnection::operator=(const ClientConnection& other) {
	if (this != &other) {
		_client_socket = other._client_socket;
		_client_address = other._client_address;
		_server = other._server;  // shallow copy (non-owning)
		_last_msg_time = other._last_msg_time;
		// _request = other._request;
		_request_header_found = other._request_header_found;
		_request_error = other._request_error;
		_msg_sent = other._msg_sent;
		_bytes_sent = other._bytes_sent;
		_request_buffer = other._request_buffer;
		// _response = other._response;
	}
	return *this;
}



ClientConnection::~ClientConnection()
{
	closeConnection();
}

void ClientConnection::setSocket(int socket)
{
	_client_socket = socket;
}

void ClientConnection::setAddress(const struct sockaddr_in &addr)
{
	_client_address = addr;
}

void ClientConnection::setServer(ServerConfig &server)
{
	_server = &server;
}

void ClientConnection::updateTime()
{
	_last_msg_time = std::time(NULL);
}

int ClientConnection::getSocket() const
{
	return _client_socket;
}

const struct sockaddr_in& ClientConnection::getAddress() const
{
	return _client_address;
}

time_t ClientConnection::getLastTime() const
{
	return _last_msg_time;
}

ServerConfig* ClientConnection::getServer() const
{
	return _server;
}

bool 	ClientConnection::getRequestIsComplete() const
{
	return _request.is_complete();
}

bool ClientConnection::getRequestError() const
{
	return _request_error;
}

bool ClientConnection::getResponseReady() const
{
	return _response.is_response_ready();
}

bool ClientConnection::getMsgSent() const
{
	return _msg_sent;
}

HTTPRequest& ClientConnection::getRequest()
{
	return _request;
}

int ClientConnection::parseReadEvent(std::string &buffer)
{
	for (;;) {
		try {
			buffer.erase(0, _request.process_header_line(buffer));
		} catch (const std::invalid_argument &e) {
			print_err("Invalid request format: ", e.what(), "");
			return 400;
		} catch (const std::range_error &e) {
			print_err("Request parsing error: ", e.what(), "");
			return 400;
		}
		if (this->_request.is_complete())
		{
			return 0;
		}
	}
}
/**
 * @brief Returns the length of the HTTP header in the request buffer.
 *
 * Finds the end-of-header delimiter ("\r\n\r\n") and returns its position
 * plus the delimiter length. Returns -1 if the header is incomplete.
 *
 * @param requestBuffer Raw HTTP request data.
 * @return int Length of the header including the delimiter, or -1 if not found.
 */
int ClientConnection::getHttpHeaderLength(const std::string& requestBuffer) {
    	std::string headerEnd = "\r\n\r\n";
    	std::size_t pos = requestBuffer.find(headerEnd);

    	if (pos == std::string::npos) {
    	    return -1;  // Header not complete
    	}

    	// Return length including the 4-byte delimiter
    	return static_cast<int>(pos + headerEnd.length());
}


/**
 * @brief Handles the read event for the client connection.
 * Reads data from the socket until no more data is available or an error occurs.
 * Parses the request from the received data and updates the last message time.
 *
 * @return true if data was successfully read and parsed, false if an error occurred or the client closed the connection.
 */
bool ClientConnection::handleReadEvent()
{
	// std::cout <<"Client header bytes: "<< _server->getLargeClientHeaderTotalBytes()<< std::endl;
	enum { BUFFER_SIZE = 2048 }; // 2 KB buffer size for reading data
        print_log("handleReadEvent() called for fd ", to_string(_client_socket), "");
	char buffer[BUFFER_SIZE];
	ssize_t n = recv(_client_socket, buffer, BUFFER_SIZE, 0);
	if (n < 0) {
		print_err("recv() failed: ", strerror(errno), "");
		return false;
	}
	if (n == 0) {
		print_log("Client closed connection", "", "");
		return false;
	}
	if (n > 0) {
	        print_log("DEBUG: Received request (normal): ", std::string(buffer, static_cast<size_t>(n)), "");

        }
	_request_buffer.append(buffer, static_cast<size_t>(n));

	int header_length = getHttpHeaderLength(_request_buffer);
	if (header_length < 0) {
		if (_request_buffer.size() > _server->getLargeClientHeaderTotalBytes()) {
			_request_error = true;
			_response.set_status_code(431);
			_response.build_error_response(*_server);
			print_err("Request too large, size: ", to_string(_request_buffer.size()), "");
			return true; // Request too large, send error response
		}
		return true; // Not enough data yet, wait for more

	}
	else if (header_length > static_cast<int>(_server->getLargeClientHeaderTotalBytes())) {
		_request_error = true;
		_response.set_status_code(431);
		_response.build_error_response(*_server);
		print_err("Request too large, size: ", to_string(_request_buffer.size()), "");
		return true; // Request too large, send error response
	}
	// We have a complete header, now parse it
	int status = parseReadEvent(_request_buffer);

	if (status != 0) {
		_request_error = true;
		_response.set_status_code(status);
		_response.build_error_response(*_server);
	}

	return true;
}

bool	ClientConnection::handleWriteEvent()
{
	const std::string& response_msg = _response.get_response_msg();
	if (response_msg.empty()) {
		print_err("Smth went wrong. No response to send", "", "");
		return false; // Nothing to send, skip write event
	}
	size_t total_size = response_msg.size();
	if (_bytes_sent >= total_size) {
		print_warning("All bytes already sent, but EPOLLOUT fired", "", "");
		return true; // Defensive, shouldn't happen
	}
	const char* data_ptr = response_msg.c_str() + _bytes_sent;
	size_t remaining = total_size - _bytes_sent;
	ssize_t n = send(_client_socket, data_ptr, remaining, 0);
	if (n < 0) {
		// Treat all n < 0 as retry later (since errno is forbidden)
		print_log("send() would block or failed, will retry", "", "");
		return true;
	}
	if (n == 0) {
		print_log("Client closed connection", "", "");
		return false;
	}

	_bytes_sent += static_cast<size_t>(n);
	if (_bytes_sent == total_size) {
		print_log("Response fully sent", "", "");
		_bytes_sent = 0;
		_msg_sent = true;
	}

	return true;
}

void ClientConnection::printDebugRequestParse(){
	_request.printDebug();
}


void ClientConnection::reset()
{
	_response.reset();
	_request.reset(); // Clear request data
	// _request_buffer.clear(); // Clear request buffer
	_request_error = false; // Reset request error state
	_msg_sent = false; // Reset message sent flag
	_bytes_sent = 0; // Reset bytes sent counter
}

void ClientConnection::closeConnection()
{
	if (_client_socket >= 0) {
		close(_client_socket);
		print_log("Client Socket fd: ", to_string(_client_socket), " closed.");
		_client_socket = -1;
	}
}

