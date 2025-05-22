#include "../include/Location.hpp"

Location::Location()
	: _path(""), _root(""), _autoindex(false), _methods(5, 0),
	  _return(""), _alias(""), _client_max_body_size(MAX_CONTENT_LENGTH) {
	_methods[0] = 1; // Enable GET by default
}

Location::Location(const Location &other)
	: _path(other._path), _root(other._root), _autoindex(other._autoindex),
	  _index(other._index), _methods(other._methods), _return(other._return),
	  _alias(other._alias), _client_max_body_size(other._client_max_body_size),
	  _cgi_path(other._cgi_path), _cgi_ext(other._cgi_ext), _ext_path(other._ext_path) {}

Location &Location::operator=(const Location &src) {
	if (this != &src) {
		_path = src._path;
		_root = src._root;
		_autoindex = src._autoindex;
		_index = src._index;
		_methods = src._methods;
		_return = src._return;
		_alias = src._alias;
		_client_max_body_size = src._client_max_body_size;
		_cgi_path = src._cgi_path;
		_cgi_ext = src._cgi_ext;
		_ext_path = src._ext_path;
	}
	return *this;
}

Location::~Location() {}

void Location::setPath(std::string param) {_path = param;}
void Location::setRootLocation(std::string param) {_root = param;}

void Location::setAutoindex(std::string param) {
	if (param == "on") _autoindex = true;
	else if (param == "off") _autoindex = false;
	else throw ConfigParser::ErrorException("Invalid autoindex value: " + param);
}

void Location::addIndexLocation(std::string param) {_index.push_back(param);}
void Location::setReturn(std::string param) {_return = param;}
void Location::setAlias(std::string param) {_alias = param;}

void Location::setCgiPath(std::string path) {
	_cgi_path.clear();
	std::stringstream ss(path);
	std::string token;
	while (ss >> token) {
		_cgi_path.push_back(token);
	}
}

void 	Location::printDebug() const{
	std::cout << "=== Location Debug Info ===" << std::endl;
		std::cout << "Path: " << _path << std::endl;
		std::cout << "Root: " << _root << std::endl;
		std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
		std::cout << "Max Body Size: " << _client_max_body_size << std::endl;

		std::cout << "Allowed Methods: ";
		for (size_t i = 0; i < _methods.size(); ++i) {
			switch (_methods[i]) {
				case 0: std::cout << "GET"; break;
				case 1: std::cout << "POST"; break;
				case 2: std::cout << "DELETE"; break;
				default: std::cout << "UNKNOWN"; break;
			}
			if (i < _methods.size() - 1)
				std::cout << ", ";
		}
		std::cout << std::endl;

		std::cout << "Index Files: ";
		for (size_t i = 0; i < _index.size(); ++i) {
			std::cout << _index[i];
			if (i < _index.size() - 1)
				std::cout << ", ";
		}
		std::cout << std::endl;

		if (!_return.empty())
			std::cout << "Return: " << _return << std::endl;
		if (!_alias.empty())
			std::cout << "Alias: " << _alias << std::endl;

		if (!_cgi_path.empty()) {
			std::cout << "CGI Paths: ";
			for (size_t i = 0; i < _cgi_path.size(); ++i) {
				std::cout << _cgi_path[i];
				if (i < _cgi_path.size() - 1)
					std::cout << ", ";
			}
			std::cout << std::endl;
		}

		if (!_cgi_ext.empty()) {
			std::cout << "CGI Extensions: ";
			for (size_t i = 0; i < _cgi_ext.size(); ++i) {
				std::cout << _cgi_ext[i];
				if (i < _cgi_ext.size() - 1)
					std::cout << ", ";
			}
			std::cout << std::endl;
		}

		if (!_ext_path.empty()) {
			std::cout << "Extension Path Mapping:" << std::endl;
			for (std::map<std::string, std::string>::const_iterator it = _ext_path.begin(); it != _ext_path.end(); ++it) {
				std::cout << "  " << it->first << " => " << it->second << std::endl;
			}
		}

		std::cout << "===========================" << std::endl;
}

void Location::addCgiExtension(std::string extension) {
	_cgi_ext.push_back(extension);
}

void Location::setMaxBodySize(std::string param) {
    if (param.empty()) {
        throw ConfigParser::ErrorException("client_max_body_size cannot be empty");
    }

    char suffix = param[param.size() - 1];
    unsigned long multiplier = 1;

    if (suffix == 'K' || suffix == 'M' || suffix == 'G') {
        param = param.substr(0, param.size() - 1);
        if (suffix == 'K') multiplier = 1024UL;
        else if (suffix == 'M') multiplier = 1024UL * 1024;
        else if (suffix == 'G') multiplier = 1024UL * 1024 * 1024;
    }

    std::stringstream ss(param);
    unsigned long size;
    ss >> size;

    if (ss.fail() || !ss.eof()) {
        throw ConfigParser::ErrorException("Invalid client_max_body_size: " + param + suffix);
    }

    _client_max_body_size = size * multiplier;
}


void Location::setMaxBodySize(unsigned long param) {
	_client_max_body_size = param;
}

void Location::resetMethode() {
	_methods.assign(5, 0); // reset to all 0s
}

void Location::addMethode(size_t index) {
	_methods[index] = 1;
}

// Getters

const std::string &Location::getPath() const { return _path; }
const std::string &Location::getRootLocation() const { return _root; }
const std::vector<short> &Location::getMethods() const { return _methods; }
const bool &Location::getAutoindex() const { return _autoindex; }
const std::vector<std::string> &Location::getIndexLocation() const { return _index; }
const std::string &Location::getReturn() const { return _return; }
const std::string &Location::getAlias() const { return _alias; }
const std::vector<std::string> &Location::getCgiPath() const { return _cgi_path; }
const std::vector<std::string> &Location::getCgiExtension() const { return _cgi_ext; }
const std::map<std::string, std::string> &Location::getExtensionPath() const { return _ext_path; }
const unsigned long &Location::getMaxBodySize() const { return _client_max_body_size; }
