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

	// We need to keep track if `_client_max_body_size`
	// was set to 0 or not set at all.
	bool				_client_max_body_size_set;

	std::vector<std::string> 	_cgi_path;
	std::vector<std::string> 	_cgi_ext;
	std::map<int, std::string> 	_error_pages;
	std::string 			_upload_path; // Path for file uploads, if applicable



public:
	Location();
	Location(const Location& other);
	Location& operator=(const Location& other);
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
	void 						setErrorPages(const std::map<int, std::string>& errorPages);
	void 						setErrorPage(int code, const std::string& path);
	void 						setUploadPath(const std::string& path);

	const std::string 				&getPath() const;
	const std::string 				&getRootLocation() const;
	const std::set<std::string>			&getMethods() const;
	const bool 					&getAutoindex() const;
	const std::vector<std::string> 			&getIndexLocation() const;
	std::pair<int, std::string> 			getReturn() const;
	const std::string 				&getAlias() const;
	const std::vector<std::string> 			&getCgiPath() const;
	const std::vector<std::string> 			&getCgiExtension() const;

	/**
	 * Get the `_client_max_body_size`.
	 * @throw	domain_error	_client_max_body_size_set == false.
	 * @return	_client_max_body_size.
	 */
	const uint64_t	 				&getMaxBodySize() const;

	const std::map<int, std::string> 		&getErrorPages() const;
	std::string 					getErrorPage(int code) const;
	const std::string 				&getUploadPath() const;

	void 						validateLocation() const;

	void 						printDebug() const;
};
