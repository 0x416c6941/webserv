#include "../include/ConfigParser.hpp"

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
		parser.print();
	} 
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return(0);
}