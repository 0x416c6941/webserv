#pragma once
#include "Webserv.hpp"
#include "ConfigParser.hpp"

/**
 * @class Location
 * @brief Represents configuration settings for a specific URI path within a server block.
 *
 * The Location class defines the behavior and rules for a particular location or route on the server,
 * including its root path, allowed HTTP methods, directory listing, index files, CGI handling, and more.
 */
class Location {
private:
	std::string 			_path;
	std::string 			_root;
	bool 				_autoindex;
	std::vector<std::string> 	_index;
	std::set<std::string>	 	_methods;
	std::pair<int, std::string> 	_return;
	std::string 			_alias;
	uint64_t 			_client_max_body_size;
	std::vector<std::string> 	_cgi_path;
	std::vector<std::string> 	_cgi_ext;
	
	public:
	Location();
	Location(const Location& other);
	~Location();
	
	// Setters
	void 						setPath(const std::string& path);
	void 						setRootLocation(const std::string& root);
	void 						setAutoindex(bool value);  
	void 						addIndexLocation(const std::string& index);
	void 						setReturn(const std::pair<int, std::string> _returnvalue);
	void 						setAlias(const std::string& alias);
	void 						addCgiPath(const std::string& path);
	void 						addCgiExtension(const std::string& ext);
	void 						setMaxBodySize(uint64_t size);
	void 						resetMethods();
	void 						addMethod(const std::string& method);
	
	const std::string 				&getPath() const;
	const std::string 				&getRootLocation() const;
	const std::set<std::string>			&getMethods() const;
	const bool 					&getAutoindex() const;
	const std::vector<std::string> 			&getIndexLocation() const;
	std::pair<int, std::string> 			getReturn() const;
	const std::string 				&getAlias() const;
	const std::vector<std::string> 			&getCgiPath() const;
	const std::vector<std::string> 			&getCgiExtension() const;
	const uint64_t	 				&getMaxBodySize() const;

	
	void 						printDebug() const;
};
