#include "../include/Location.hpp"

Location::Location()
    : _path(""), _root(""), _autoindex(false),
      _return(""), _alias(""), _client_max_body_size(MAX_CONTENT_LENGTH) 
{
    _methods["GET"] = true;     
    _methods["POST"] = false;
    _methods["PUT"] = false;
    _methods["DELETE"] = false;
    _methods["HEAD"] = false;
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

void Location::addCgiPath(std::string path) {
	std::stringstream ss(path);
	std::string token;
	while (ss >> token) {
		_cgi_path.push_back(token);
	}
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
    for (std::map<std::string, bool>::iterator it = _methods.begin(); it != _methods.end(); ++it) {
        it->second = false;
    }
}


void Location::addMethode(const std::string& method) {
    if (_methods.find(method) != _methods.end()) {
        _methods[method] = true;
    } else {
        throw ConfigParser::ErrorException("Unsupported HTTP method: " + method);
    }
}


// Getters

const std::string &Location::getPath() const { return _path; }
const std::string &Location::getRootLocation() const { return _root; }
const std::map<std::string, bool>& Location::getMethods() const { return _methods; }
const bool &Location::getAutoindex() const { return _autoindex; }
const std::vector<std::string> &Location::getIndexLocation() const { return _index; }
const std::string &Location::getReturn() const { return _return; }
const std::string &Location::getAlias() const { return _alias; }
const std::vector<std::string> &Location::getCgiPath() const { return _cgi_path; }
const std::vector<std::string> &Location::getCgiExtension() const { return _cgi_ext; }
const unsigned long &Location::getMaxBodySize() const { return _client_max_body_size; }



void 	Location::printDebug() const{
	std::cout << "=== Location Debug Info ===" << std::endl;
		std::cout << "Path: " << _path << std::endl;
		std::cout << "Root: " << _root << std::endl;
		std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
		std::cout << "Max Body Size: " << _client_max_body_size << std::endl;

		std::cout << "Allowed Methods: ";
   		 bool first = true;
   		 for (std::map<std::string, bool>::const_iterator it = _methods.begin(); it != _methods.end(); ++it) {
   		     if (it->second) {
   		         if (!first) std::cout << ", ";
   		         std::cout << it->first;
   		         first = false;
   		     }
   		 }
   		 if (first) {
   		     std::cout << "(none)";
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
		std::cout << "===========================" << std::endl;
}
