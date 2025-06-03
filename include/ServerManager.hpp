#pragma once
#include "Webserv.hpp"
#include "ServerConfig.hpp"

class ServerManager {
private:
	    std::vector<ServerConfig> _servers;
	    int _epoll_fd;
	    std::map<int, ServerConfig*> _fd_to_server;

public:
	    ServerManager();
	    ~ServerManager();
    
	    void loadServers(const std::vector<ServerConfig>& servers);
	    void init();
	    void run(); // Will later include event loop handling.
    
	    // Helpers
	    int getEpollFd() const;
	    void cleanup();
};
