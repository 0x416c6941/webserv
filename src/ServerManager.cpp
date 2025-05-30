#include "../include/ServerManager.hpp"

ServerManager::ServerManager() : _epoll_fd(-1) {}

ServerManager::~ServerManager() {
	cleanup();
}

void ServerManager::loadServers(const std::vector<ServerConfig>& servers) {
	_servers = servers;
}

void ServerManager::init() {
	_epoll_fd = epoll_create(1);
	if (_epoll_fd < 0) {
		throw std::runtime_error("Failed to create epoll instance: " + std::string(strerror(errno)));
	}

	for (size_t i = 0; i < _servers.size(); ++i) {
		try {
			_servers[i].initServer();
			const std::vector<int>& fds = _servers[i].getListenFds();
			for (size_t j = 0; j < fds.size(); ++j) {
				int fd = fds[j];

				// Set non-blocking
				if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
					throw std::runtime_error("Failed to set non-blocking mode on fd " + to_string(fd));
				}

				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.fd = fd;

				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
					throw std::runtime_error("Failed to add fd to epoll: " + to_string(fd));
				}

				_fd_to_server[fd] = &_servers[i];

				print_log("Listening socket ", to_string(fd), " registered with epoll");
			}
		} catch (const std::exception& e) {
			_servers[i].cleanupSocket();
			print_err("Server init failed: ", e.what(), "");
			continue;
		}
	}

	if (_fd_to_server.empty()) {
		throw std::runtime_error("No valid servers were initialized");
	}
}

void ServerManager::cleanup() {
	for (size_t i = 0; i < _servers.size(); ++i)
		_servers[i].cleanupSocket();

	if (_epoll_fd >= 0) {
		close(_epoll_fd);
		_epoll_fd = -1;
	}
}

int ServerManager::getEpollFd() const {
	return _epoll_fd;
}

void ServerManager::run() {
	print_log("", "ServerManager event loop starting...", "");

	// Placeholder: actual event handling would go here.
	// Example: epoll_wait(...)

	print_log("", "ServerManager event loop finished.", "");
}
