#pragma once
#include "Webserv.hpp"
#include "ConfigParser.hpp"

class Location {
private:
	std::string 			_path;
	std::string 			_root;
	bool 				_autoindex;
	std::vector<std::string> 	_index;
	std::map<std::string, bool> 	_methods;
	std::string 			_return;
	std::string 			_alias;
	unsigned long 			_client_max_body_size;
	std::vector<std::string> 	_cgi_path;
	std::vector<std::string> 	_cgi_ext;
	
	public:
	std::map<std::string, std::string> _ext_path;
	
	Location();
	~Location();
	
	void 						setPath(std::string);
	void 						setRootLocation(std::string);
	void 						setAutoindex(std::string);
	void 						addIndexLocation(std::string);
	void 						setReturn(std::string);  
	void 						setAlias(std::string);
	void 						addCgiPath(std::string);  
	void 						addCgiExtension(std::string);
	void 						setMaxBodySize(std::string);
	void 						setMaxBodySize(unsigned long);
	void 						resetMethode();
	void 						addMethode(const std::string& method);
	
	const std::string 				&getPath() const;
	const std::string 				&getRootLocation() const;
	const std::map<std::string, bool>		&getMethods() const;
	const bool 					&getAutoindex() const;
	const std::vector<std::string> 			&getIndexLocation() const;
	const std::string 				&getReturn() const;
	const std::string 				&getAlias() const;
	const std::vector<std::string> 			&getCgiPath() const;
	const std::vector<std::string> 			&getCgiExtension() const;
	const std::map<std::string, std::string> 	&getExtensionPath() const;
	const unsigned long 				&getMaxBodySize() const;

	
	void 						printDebug() const;
};
