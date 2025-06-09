#include "../include/ServerManager.hpp"

static volatile sig_atomic_t g_shutdown_requested = 0;

extern "C" void handle_signal(int sig)
{
	(void)sig;
	print_log("", "Received shutdown signal, cleaning up...", "");
	g_shutdown_requested = 1;
}


ServerManager::ServerManager() : _epoll_fd(-1) {}

ServerManager::~ServerManager() {
	cleanup();
}

void ServerManager::loadServers(const std::vector<ServerConfig>& servers) {
	_servers = servers;
}

void ServerManager::initializeSockets() {
	_epoll_fd = epoll_create(1);
	if (_epoll_fd < 0) {
		// print_err("Failed to create epoll instance: ", strerror(errno), "");
		throw std::runtime_error("Failed to create epoll instance: " + std::string(strerror(errno)));
	}

	for (size_t i = 0; i < _servers.size(); ++i) {
		try {
			_servers[i].initServerSocket();
			const std::vector<int>& fds = _servers[i].getListenFds();
			if (fds.empty()) {
				print_warning("No listening sockets found for server ", to_string(i), "");
				continue;
			}

			for (size_t j = 0; j < fds.size(); ++j) {
				int fd = fds[j];

				// Set non-blocking
				if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
					throw std::runtime_error("Failed to set non-blocking mode on fd: " + to_string(fd));
				}

				if (!addFdToEpoll(fd, EPOLLIN)) {
					throw std::runtime_error("Failed to add fd to epoll: " + to_string(fd));
				}

				_fd_to_server[fd] = &_servers[i];

				print_log("Listening socket ", to_string(fd), " registered with epoll");
			}
		} catch (const std::exception& e) {
			_servers[i].cleanupSocket();
			print_err("Server initializeSockets failed: ", e.what(), "");
			continue;
		}
	}

	if (_fd_to_server.empty()) {
		throw std::runtime_error("No valid servers were initialized");
	}
}

void ServerManager::cleanup()
{
	print_log("", "Cleaning up server sockets...", "");

	for (size_t i = 0; i < _servers.size(); ++i) {
		_servers[i].cleanupSocket();
	}

	_fd_to_server.clear();
	_client_connections.clear();

	if (_epoll_fd >= 0) {
		print_log("", "Closing epoll file descriptor...", "");
		close(_epoll_fd);
		_epoll_fd = -1;
	}

	print_log("", "Cleanup complete.", "");
}


int ServerManager::getEpollFd() const {
	return _epoll_fd;
}


/**
 * @brief Accepts and registers new incoming client connections.
 *
 * This function is triggered when the epoll instance reports a readable event
 * on a server (listening) socket. It accepts all pending client connections
 * in a non-blocking loop using `accept()`, sets each new client socket to
 * non-blocking mode, and registers it with the server's epoll instance
 * using edge-triggered (EPOLLET) mode.
 *
 * For each successfully accepted connection, a ClientConnection object is
 * created, initialized with socket information and associated ServerConfig,
 * and stored in the _client_connections map.
 *
 * If accept() returns EAGAIN or EWOULDBLOCK, the function breaks the loop,
 * assuming all pending connections have been processed.
 *
 * In case of error (e.g., failed fcntl, epoll_ctl, etc.), the client socket
 * is closed and skipped without crashing the server.
 *
 * @param server_fd File descriptor of the server's listening socket that
 *        received the connection request.
 */
void ServerManager::handleNewConnection(int server_fd)
{
	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
		if (client_fd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			print_err("accept() failed: ", strerror(errno), "");
			break;
		}

		// Set client socket to non-blocking mode
		int flags = fcntl(client_fd, F_GETFL, 0);
		if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
			print_err("fcntl() failed: ", strerror(errno), "");
			::close(client_fd);
			print_warning("Failed to set non-blocking mode for client fd ", to_string(client_fd), "");
			continue;
		}


		// Register client fd with epoll
		if (!addFdToEpoll(client_fd, EPOLLIN | EPOLLET)) {
			::close(client_fd);
			print_err("Failed to add client fd to epoll: ", to_string(client_fd), "");
			continue;
		}

		// Create and store new client connection
		ClientConnection conn;
		conn.setSocket(client_fd);
		conn.setAddress(client_addr);

		std::map<int, ServerConfig*>::iterator sit = _fd_to_server.find(server_fd);
		if (sit != _fd_to_server.end())
			conn.setServer(*sit->second);

		_client_connections[client_fd] = conn;

		print_log("Accepted connection: fd ", to_string(client_fd), "");
	}
}


/**
 * @brief Handles an incoming client event on a given file descriptor.
 *
 * This function is called when epoll signals activity on a client socket.
 * It retrieves the associated ClientConnection object and attempts to process
 * the incoming data by calling ClientConnection::handleRead().
 *
 * If handleRead() returns false, it indicates that the client has closed the
 * connection, there was an error, or the server should close the connection
 * (e.g., after processing a non-persistent HTTP request).
 *
 * In such cases, this function:
 * - Removes the file descriptor from the epoll instance.
 * - Closes the underlying socket using ClientConnection::closeConnection().
 * - Erases the client from the internal _client_connections map.
 * - Logs the closure for debugging purposes.
 *
 * @param client_fd The file descriptor for the client socket that triggered the event.
 */
void ServerManager::handleClientEvent(int client_fd)
{
	std::map<int, ClientConnection>::iterator it = _client_connections.find(client_fd);
	if (it == _client_connections.end()) {
		print_warning("Event for unknown client fd: ", to_string(client_fd), "");
		return; // the connection was probably closed
	}
	ClientConnection &conn = it->second;
	if (!conn.handleRead()) {
		closeClientConnection(client_fd);
	}

}

void ServerManager::closeClientConnection(int client_fd)
{
	std::map<int, ClientConnection>::iterator it = _client_connections.find(client_fd);
	if (it == _client_connections.end())
		return;

	if (!removeFdFromEpoll(client_fd)) {
		print_warning("Failed to remove fd: ", to_string(client_fd), " from epoll");
	}

	it->second.closeConnection();
	_client_connections.erase(it);

	print_log("Closed connection: fd ", to_string(client_fd), "");
}


bool ServerManager::addFdToEpoll(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		print_err("epoll_ctl(ADD) failed: ", strerror(errno), "");
		return false;
	}
	return true;
}

bool ServerManager::removeFdFromEpoll(int fd)
{
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		print_warning("epoll_ctl(DEL) failed: ", strerror(errno), "");
		return false;
	}
	return true;
}



void ServerManager::run() {
        struct epoll_event events[EPOLL_MAX_EVENTS];
	print_log("", "ServerManager event loop starting...", "");
        while (!g_shutdown_requested) {
                int n = epoll_wait(_epoll_fd, events, EPOLL_MAX_EVENTS, -1);
                if (n < 0) {
			if (errno == EINTR) {
				// Interrupted by signal — check shutdown flag and continue
				continue;
			}
			print_err("epoll_wait failed: ", strerror(errno), "");
			break;
		}

                for (int i = 0; i < n; ++i) {
                        int fd = events[i].data.fd;

                        if (_fd_to_server.count(fd)) {
                                handleNewConnection(fd);
                        } else {
                                handleClientEvent(fd);
                        }
                }
        }
	print_log("", "Shutdown requested. Cleaning up...", "");
	cleanup();
	print_log("", "ServerManager event loop finished.", "");
}



void ServerManager::setupSignalHandlers()
{
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) < 0)
		print_err("Failed to set SIGINT handler: ", strerror(errno), "");

	if (sigaction(SIGTERM, &sa, NULL) < 0)
		print_err("Failed to set SIGTERM handler: ", strerror(errno), "");

	print_log("", "Signal handlers installed (SIGINT, SIGTERM)", "");
}
