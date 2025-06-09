#include "../include/ConfigParser.hpp"
#include "../include/ServerManager.hpp"
#include "../include/Webserv.hpp"

int main(int argc, char **argv)
{
	if (argc > 2) {
		std::cerr << "Error: Usage: ./webserv [optional: path to config file]" << std::endl;
		return 1;
	}

	try {
		ServerManager::setupSignalHandlers();
		std::string cfg_path = (argc == 1 ? "configs/default.conf" : argv[1]);

		// Step 1: Parse configuration
		ConfigFile cfg_file(cfg_path);
		ConfigParser parser(cfg_file.readContent());
		parser.parse();
		std::vector<ServerConfig> servers = parser.getServers();

		// Step 2: Initialize ServerManager and servers
		ServerManager manager;
		manager.loadServers(servers);
		manager.initializeSockets(); // Initializes all sockets and epoll registration

		// Step 3: Optionally print debug info
		if (DEBUG) {
			for (size_t i = 0; i < servers.size(); ++i) {
				std::cout << "---- Server ----" << std::endl;
				printServerConfig(servers[i]);
			}
		}

		// Step 4: Run the event loop (stub for now)
		manager.run();
	}
	catch (const std::exception& e) {
		print_err("Fatal error: ", e.what(), "");
		return 1;
	}

	return 0;
}
