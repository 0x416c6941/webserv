#include "../include/ConfigParser.hpp"
#include "../include/Webserv.hpp"

int main(int argc, char **argv)
{
	if (argc > 2){
		std::cerr << "Error: Usage: ./webserv or ./webserv[path to cfg file]" << std::endl;
		return(1);
	}
	
	try {
		std::string cfg_path  = (argc == 1 ? "configs/default.conf" : argv[1]);
		ConfigFile cfg_file(cfg_path);
		ConfigParser parser(cfg_file.readContent());
		parser.parse();
		std::vector<ServerConfig> servers = parser.getServers();
		if (DEBUG)
		{
			for (std::vector<ServerConfig>::iterator it = servers.begin(); it != servers.end(); ++it)
			{
				std::cout << "---- Server ----" << std::endl; 
				printServerConfig(*it);
			}
		}
		// for (std::vector<ServerConfig>::iterator it = servers.begin(); it != servers.end(); ++it)
		// {
		// 	it->initServer();
		// }

		
	} 
	catch (const std::exception& e) {
		print_err("Fatal error: ", e.what(), "");
		return 1;
	}

	return(0);
}