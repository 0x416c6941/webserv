#include "../include/Location.hpp"

Location::Location()
	: _path(""),
	  _root(""),
	  _autoindex(false),
	  _return(std::make_pair(0, "")),
	  _alias(""),
	  _client_max_body_size(DEFAULT_CONTENT_LENGTH) {}


Location::~Location() {}

// Setters
void Location::setPath(const std::string& path) { _path = path; }
void Location::setRootLocation(const std::string& root) { _root = root; }
void Location::setAutoindex(bool value) { _autoindex = value; }
void Location::addIndexLocation(const std::string& index) { _index.push_back(index); }
void Location::setReturn(const std::pair<int, std::string> returnValue) { _return = returnValue; }
void Location::setAlias(const std::string& alias) { _alias = alias; }
void Location::addCgiPath(const std::string& path) { _cgi_path.push_back(path); }
void Location::addCgiExtension(const std::string& ext) { _cgi_ext.push_back(ext); }
void Location::setMaxBodySize(uint64_t size) { _client_max_body_size = size; }
void Location::resetMethods() {	_methods.clear(); }
void Location::addMethod(const std::string& method) { _methods.insert(method); }

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
