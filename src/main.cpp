#include "../include/Webserv.hpp"

int main(int argc, char **argv)
{
	if (argc > 2){
		std::cerr << "Error: Usage: ./webserv or ./webserv[path to cfg file]" << std::endl;
		return(1);
	}
	
	try {
		std::string cfg_path;
		cfg_path  = (argc == 1 ? "configs/default.conf" : argv[1]);
		ConfigFile config(cfg_path);

		// Will throw if the file is missing or not readable
		config.validateFile();

		std::string content = config.readFile();
		std::cout << "Config file content:\n" << content << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return(0);
}
