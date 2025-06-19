#include "../include/ClientConnection.hpp"


ClientConnection::ClientConnection()
	: _client_socket(-1), _server(NULL), _last_msg_time(std::time(NULL))
{
	std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection(int fd)
	: _client_socket(fd), _server(NULL), _last_msg_time(std::time(NULL))
{
	std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection(const ClientConnection &other)
	: _client_socket(other._client_socket),
	  _client_address(other._client_address),
	  _server(other._server),
	  _last_msg_time(other._last_msg_time)
{
	// _request = other._request;
	// _response = other._response;
}


ClientConnection &ClientConnection::operator=(const ClientConnection &rhs)
{
	if (this != &rhs) {
		_client_socket = rhs._client_socket;
		_client_address = rhs._client_address;
		_server = rhs._server; // non-owning pointer, shallow copy
		_last_msg_time = rhs._last_msg_time;

		// If HttpRequest/Response get added, copy them too
		// _request = rhs._request;
		// _response = rhs._response;
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

bool ClientConnection::handleReadEvent()
{
	enum { BUFFER_SIZE = 1024 };
        print_log("handleReadEvent() called for fd ", to_string(_client_socket), "");
	char buffer[BUFFER_SIZE];
	while (true) {
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
	                print_log("DEBUG: Received request: ", std::string(buffer, static_cast<size_t>(n)), "");
                }


		const char *response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: 13\r\n"
			"Content-Type: text/plain\r\n"
			// "Connection: closed\r\n"
			"\r\n"
			"Hello, world!";

		send(_client_socket, response, std::strlen(response), 0);
		return true; // Only one-shot response for now
	}
	return true;
}



void ClientConnection::closeConnection()
{
	if (_client_socket >= 0) {
		close(_client_socket);
		print_log("Client Socket fd: ", to_string(_client_socket), " closed.");
		_client_socket = -1;
	}
}

