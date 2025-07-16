#include "../include/ClientConnection.hpp"

ClientConnection::ClientConnection(int fd)
	: _client_socket(fd),
	  _client_address(),
	  _server(NULL),
	  _last_msg_time(std::time(NULL)),
	  _request_error(false),
	  _msg_sent(false),
	  _bytes_sent(0),
	  _request_buffer(),
	  _header_buffer_bytes_exhausted(0),
	  _body_buffer_bytes_exhausted(0),
	  _response()
{
    std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection()
	: _client_socket(-1),
	  _client_address(),
	  _server(NULL),
	  _last_msg_time(std::time(NULL)),
	  _request_error(false),
	  _msg_sent(false),
	  _bytes_sent(0),
	  _request_buffer(),
	  _header_buffer_bytes_exhausted(0),
	  _body_buffer_bytes_exhausted(0),
	  _response()
{
    std::memset(&_client_address, 0, sizeof(_client_address));
}

ClientConnection::ClientConnection(const ClientConnection &other)
	: _client_socket(other._client_socket),
	  _client_address(other._client_address),
	  _server(other._server),
	  _last_msg_time(other._last_msg_time),
	  _request_error(other._request_error),
	  _msg_sent(other._msg_sent),
	  _bytes_sent(other._bytes_sent),
	  _request_buffer(other._request_buffer),
	  _header_buffer_bytes_exhausted(other._header_buffer_bytes_exhausted),
	  _body_buffer_bytes_exhausted(other._body_buffer_bytes_exhausted),
	  _response(other._response)
{
}

ClientConnection::~ClientConnection()
{
	closeConnection();
}

int ClientConnection::parseReadEvent(std::string &buffer)
{
	const std::string HEADER_DELIM = "\r\n";
	size_t processed_bytes, max_body_size;

	if (this->_request.is_complete()) {
		throw std::range_error("ClientConnection::parseReadEvent(): Request is already fully parsed.");
	}
	while (buffer.length() > 0) {
		if (!this->_request.is_header_complete()) {
			if (buffer.find(HEADER_DELIM) == std::string::npos) {
				// Currently hold part of start line / header field
				// isn't complete.
				return 0;
			}
			// Not all exceptions should be caught in this method.
			try {
				processed_bytes = _request.process_header_line(buffer);
			}
			catch (const std::invalid_argument &e) {
				print_err("Invalid request format: ", e.what(), "");
				return 400;
			}
			catch (const std::runtime_error &e) {
				print_err("Malformed request: ", e.what(), "");
				return 400;
			}
			catch (const HTTPRequest::method_not_allowed &e)
			{
				print_err("Unsupported method: ", e.what(), "");
				return 405;
			}
			catch (const HTTPRequest::http_ver_unsupported &e)
			{
				print_err("Unsupported HTTP version: ", e.what(), "");
				return 505;
			}
			this->_header_buffer_bytes_exhausted += processed_bytes;
			buffer.erase(0, processed_bytes);
			if (this->_header_buffer_bytes_exhausted
				> this->_server->getLargeClientHeaderTotalBytes()) {
				// Request's header buffer bytes are exhausted.
				print_err("Request's header is too large, currently processed:",
				to_string(_header_buffer_bytes_exhausted), "");
				return 431;
			}
		}
		else if (this->_request.get_method() == HTTPRequest::POST
			&& !(this->_request.is_body_complete())) {
			// Before processing all body parts,
			// we should ideally check if "Content-Length" field
			// isn't bigger than "client_max_body_size".
			// Currently, we read the content until
			// `client_max_body_size` bytes are read,
			// and we throw exception only afterwards.
			//
			// Still, I believe this isn't an issue
			// for the PoC project.
			try {
				processed_bytes = _request.process_body_part(buffer);
			}
			catch (const std::invalid_argument &e) {
				// "Chunked" encoding is used,
				// however chunk isn't fully received yet.
				return 0;
			}
			catch (const std::runtime_error &e) {
				print_err("Request's body parsing error: ", e.what(), "");
				return 400;
			}
			catch (const std::domain_error &e) {
				// Have neither "Content-Length",
				// nor "Transfer-Encoding" fields in the request.
				print_err("Request's body parsing error: ", e.what(), "");
				return 411;
			}
			this->_body_buffer_bytes_exhausted += processed_bytes;
			buffer.erase(0, processed_bytes);
			try {
				max_body_size = this->getMaxBodySize(
						this->_request.get_request_target());
				if (this->_body_buffer_bytes_exhausted > max_body_size) {
					// Request's body buffer bytes are exhausted.
					print_err("Request's body is too large, currently processed: ",
						to_string(_body_buffer_bytes_exhausted), "");
					return 413;
				}
			}
			catch (const std::domain_error &e) {
				print_err("Saving a file is forbidden at: ",
					this->_request.get_request_target(), "");
				return 403;
			}
		}
		else {
			// If we got here, then request header and (or) request body
			// was (were) successfully parsed
			// and request is ready to be processed.
			return 0;
		}
	}
	// All information received in `handleReadEvent()`
	// was successfully processed and no errors were found (at least yet).
	return 0;
}

size_t ClientConnection::getMaxBodySize(const std::string &request_path) const {
	try {
		const Location &loc = _server->determineLocation(request_path);

		if (loc.getMethods().find("POST") == loc.getMethods().end()) {
			throw std::domain_error(std::string("ClientConnection::getMaxBodySize(): ")
					+ "POST method isn't allowed on the requested Location.");
		}
		try {
			return loc.getMaxBodySize();
		}
		catch (const std::domain_error &e) {
			// Max body size wasn't defined for that location.
			// Using a generic one.
			return this->_server->getClientMaxBodySize();
		}
	}
	catch (const std::out_of_range &e) {
		// Location with such path wasn't found.
		//
		// We don't support "allow_methods" directive for "server" blocks
		// and instead directly support only GET methods.
		throw std::domain_error(std::string("ClientConnection::getMaxBodySize(): ")
				+ "POST method isn't allowed on the requested Location.");
	}
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
	_response.set_server_cfg(_server);
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

size_t ClientConnection::getRequestHeaderBufferBytesExhaustion() const
{
	return _header_buffer_bytes_exhausted;
}

size_t ClientConnection::getRequestBodyBufferBytesExhaustion() const
{
	return _body_buffer_bytes_exhausted;
}

HTTPRequest& ClientConnection::getRequest()
{
	return _request;
}

bool ClientConnection::handleReadEvent()
{
	// std::cout <<"Client header bytes: "<< _server->getLargeClientHeaderTotalBytes()<< std::endl;
	enum { BUFFER_SIZE = 65536 }; // 64 KiB buffer size for reading data.

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
	_request_buffer.append(buffer, static_cast<size_t> (n));
	// Parse received information.
	int status = parseReadEvent(_request_buffer);
	if (status != 0) {
		_request_error = true;
		_response = HTTPResponse(status);
		_response.set_server_cfg(_server);
		_response.build_error_response();
	}
	return true;
}

bool	ClientConnection::handleWriteEvent()
{
	const std::string &response_msg = _response.get_response_msg();
	size_t total_size = response_msg.size();
	const char * data_ptr = response_msg.c_str() + _bytes_sent;
	size_t remaining = total_size - _bytes_sent;
	// Let's send response in packets w/ size of 64 KiB.
	enum { MAX_BYTES_TO_SEND = 65536 };
	ssize_t n;

	if (remaining <= MAX_BYTES_TO_SEND)
	{
		n = send(_client_socket, data_ptr, remaining, 0);
	}
	else
	{
		n = send(_client_socket, data_ptr, MAX_BYTES_TO_SEND, 0);
	}
	if (n < 0) {
		print_warning("send() failed", "", "");
		return false;
	}
	if (n == 0) {
		print_log("Client closed connection", "", "");
		return false;
	}
	_bytes_sent += static_cast<size_t>(n);
	if (_bytes_sent == total_size) {
		print_log("Response fully sent", "", "");
		_msg_sent = true;
	}
	return true;
}

void ClientConnection::printDebugRequestParse()
{
	_request.printDebug();
}


void ClientConnection::reset()
{
	_request_error = false; // Reset request error state
	_msg_sent = false; // Reset message sent flag
	_bytes_sent = 0; // Reset bytes sent counter
	_request_buffer.clear();
	_header_buffer_bytes_exhausted = 0;
	_body_buffer_bytes_exhausted = 0;
	_request.reset(); // Clear request data
	_response = HTTPResponse();
	_response.set_server_cfg(_server);
}

void ClientConnection::closeConnection()
{
	if (_client_socket >= 0) {
		close(_client_socket);
		print_log("Client Socket fd: ", to_string(_client_socket), " closed.");
		_client_socket = -1;
	}
}

