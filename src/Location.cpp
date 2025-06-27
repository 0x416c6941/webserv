#include "../include/Location.hpp"

Location::Location()
	: _path(""),
	  _root(""),
	  _autoindex(false),
	  _upload_enabled(false),
	  _return(std::make_pair(0, "")),
	  _alias(""),
	  _client_max_body_size(DEFAULT_CONTENT_LENGTH) {}

Location::Location(const Location& other)
	: _path(other._path),
	  _root(other._root),
	  _autoindex(other._autoindex),
	  _index(other._index),
	  _methods(other._methods),
	  _return(other._return),
	  _alias(other._alias),
	  _client_max_body_size(other._client_max_body_size),
	  _cgi_path(other._cgi_path),
	  _cgi_ext(other._cgi_ext)
{}


Location::~Location() {}

// Setters
void 					Location::setPath(const std::string& path) { _path = path; }
void 					Location::setRootLocation(const std::string& root) { _root = root; }
void 					Location::setAutoindex(bool value) { _autoindex = value; }
void 					Location::addIndexLocation(const std::string& index) { _index.push_back(index); }
void 					Location::setReturn(const std::pair<int, std::string> returnValue) { _return = returnValue; }
void 					Location::setAlias(const std::string& alias) { _alias = alias; }
void 					Location::addCgiPath(const std::string& path) { _cgi_path.push_back(path); }
void 					Location::addCgiExtension(const std::string& ext) { _cgi_ext.push_back(ext); }
void 					Location::setMaxBodySize(uint64_t size) { _client_max_body_size = size; }
void 					Location::resetMethods() {	_methods.clear(); }
void 					Location::addMethod(const std::string& method) { _methods.insert(method); }
void 					Location::setErrorPages(const std::map<int, std::string>& errorPages) { _error_pages = errorPages; }
void 					Location::setErrorPage(int code, const std::string& path) { _error_pages[code] = path; }
void 					Location::setUploadPath(const std::string& path) { _upload_path = path; }
void 					Location::setUploadEnabled(bool enabled) { _upload_enabled = enabled; }

// Getters
const std::string& 			Location::getPath() const { return _path; }
const std::string& 			Location::getRootLocation() const { return _root; }
const std::set<std::string>& 		Location::getMethods() const { return _methods; }
const bool& 				Location::getAutoindex() const { return _autoindex; }
const std::vector<std::string>& 	Location::getIndexLocation() const { return _index; }
std::pair<int, std::string> 		Location::getReturn() const { return _return; }
const std::string& 			Location::getAlias() const {	return _alias; }
const std::vector<std::string>& 	Location::getCgiPath() const { return _cgi_path; }
const std::vector<std::string>& 	Location::getCgiExtension() const { return _cgi_ext; }
const uint64_t& 			Location::getMaxBodySize() const { return _client_max_body_size; }
const std::string&			Location::getUploadPath() const{ return _upload_path; }
const std::map<int, std::string>& 	Location::getErrorPages() const { return _error_pages; }
const bool& 				Location::getUploadEnabled() const { return _upload_enabled; }

std::string 				Location::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = _error_pages.find(code);
    if (it != _error_pages.end())
        return it->second;
    return "";
}


void Location::printDebug() const {
	std::cout << "=== Location Debug Info ===" << std::endl;

	std::cout << "Path: " << _path << std::endl;
	std::cout << "Root: " << _root << std::endl;
	std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
	std::cout << "Max Body Size: " << _client_max_body_size << std::endl;

	// Methods
	std::cout << "Allowed Methods: ";
	if (_methods.empty()) {
		std::cout << "(none)";
	} else {
		std::set<std::string>::const_iterator it = _methods.begin();
		std::cout << *it;
		for (++it; it != _methods.end(); ++it) {
			std::cout << ", " << *it;
		}
	}
	std::cout << std::endl;

	// Index
	std::cout << "Index Files: ";
	if (_index.empty()) {
		std::cout << "(none)";
	} else {
		for (size_t i = 0; i < _index.size(); ++i) {
			std::cout << _index[i];
			if (i < _index.size() - 1)
				std::cout << ", ";
		}
	}
	std::cout << std::endl;

	// Return
	if (_return.first != 0 || !_return.second.empty())
		std::cout << "Return: " << _return.first << " " << _return.second << std::endl;

	// Alias
	if (!_alias.empty())
		std::cout << "Alias: " << _alias << std::endl;

	// CGI Paths
	if (!_cgi_path.empty()) {
		std::cout << "CGI Paths: ";
		for (size_t i = 0; i < _cgi_path.size(); ++i) {
			std::cout << _cgi_path[i];
			if (i < _cgi_path.size() - 1)
				std::cout << ", ";
		}
		std::cout << std::endl;
	}

	// CGI Extensions
	if (!_cgi_ext.empty()) {
		std::cout << "CGI Extensions: ";
		for (size_t i = 0; i < _cgi_ext.size(); ++i) {
			std::cout << _cgi_ext[i];
			if (i < _cgi_ext.size() - 1)
				std::cout << ", ";
		}
		std::cout << std::endl;
	}

	std::cout << "===========================" << std::endl;
}
