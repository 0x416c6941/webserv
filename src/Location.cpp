#include "../include/Location.hpp"
#include <cctype>

Location::Location()
        : _path(""),
          _root(""),
          _autoindex(false),
          _index(),
          _methods(),
          _alias(""),
          _client_max_body_size(0),
          _client_max_body_size_set(false),
          _cgi_path(),
          _cgi_ext(),
          _error_pages(),
          _upload_path("") {
}


Location& Location::operator=(const Location& other) {
        if (this != &other) {
                _path = other._path;
                _root = other._root;
                _autoindex = other._autoindex;
                _index = other._index;
                _methods = other._methods;
                _alias = other._alias;
                _client_max_body_size = other._client_max_body_size;
                _client_max_body_size_set = other._client_max_body_size_set;
                _cgi_path = other._cgi_path;
                _cgi_ext = other._cgi_ext;
                _error_pages = other._error_pages;
                _upload_path = other._upload_path;
        }
        return *this;
}


Location::Location(const Location& other)
        : _path(other._path),
          _root(other._root),
          _autoindex(other._autoindex),
          _index(other._index),
          _methods(other._methods),
          _alias(other._alias),
          _client_max_body_size(other._client_max_body_size),
          _client_max_body_size_set(other._client_max_body_size_set),
          _cgi_path(other._cgi_path),
          _cgi_ext(other._cgi_ext),
          _error_pages(other._error_pages),
          _upload_path(other._upload_path) {
}



Location::~Location() {}

// Setters
void Location::setPath(const std::string& path)
{
	if (path.length() == 0 || path.at(0) != '/'
		|| path.at(path.length() - 1) != '/')
	{
		throw std::invalid_argument(std::string("Location::setPath(): ")
				+ path + " doesn't begin or end with '/'.");
	}
	_path = path;
}
void 					Location::setRootLocation(const std::string& root) { _root = root; }
void 					Location::setAutoindex(bool value) { _autoindex = value; }
void 					Location::addIndexLocation(const std::string& index) { _index.push_back(index); }
void 					Location::setAlias(const std::string& alias) { _alias = alias; }
void 					Location::addCgiPath(const std::string& path) { _cgi_path.push_back(path); }
void 					Location::addCgiExtension(const std::string& ext)
{
	std::string ext_lowercase;

	for (size_t i = 0; i < ext.length(); i++)
	{
		ext_lowercase.push_back(std::tolower(ext.at(i)));
	}
	_cgi_ext.push_back(ext_lowercase);
}
void 					Location::setMaxBodySize(uint64_t size) {
	_client_max_body_size = size;
	_client_max_body_size_set = true;
}
void 					Location::resetMethods() {	_methods.clear(); }
void 					Location::addMethod(const std::string& method) { _methods.insert(method); }
void 					Location::setErrorPages(const std::map<int, std::string>& errorPages) { _error_pages = errorPages; }
void 					Location::setErrorPage(int code, const std::string& path) { _error_pages[code] = path; }
void 					Location::setUploadPath(const std::string& path) { _upload_path = path; }

// Getters
const std::string& 			Location::getPath() const { return _path; }
const std::string& 			Location::getRootLocation() const { return _root; }
const std::set<std::string>& 		Location::getMethods() const { return _methods; }
const bool& 				Location::getAutoindex() const { return _autoindex; }
const std::vector<std::string>& 	Location::getIndexLocation() const { return _index; }
const std::string& 			Location::getAlias() const {	return _alias; }
const std::vector<std::string>& 	Location::getCgiPath() const { return _cgi_path; }
const std::vector<std::string>& 	Location::getCgiExtension() const { return _cgi_ext; }
const uint64_t& 			Location::getMaxBodySize() const {
	if (!_client_max_body_size_set) {
		throw std::domain_error("Location::getMaxBodySize(): Option wasn't in the config.");
	}
	return _client_max_body_size;
}
const std::string&			Location::getUploadPath() const{ return _upload_path; }
const std::map<int, std::string>& 	Location::getErrorPages() const { return _error_pages; }

std::string 				Location::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = _error_pages.find(code);
    if (it != _error_pages.end())
        return it->second;
    return "";
}


/**
 * @brief Validates a non-empty directory path or throws.
 *
 * If @p path is not empty and invalid, throws with a message
 * including the given @p label.
 *
 * @param label Context label for the error (e.g., "root").
 * @param path The directory path to check.
 *
 * @throws std::runtime_error if the path is non-empty and invalid.
 */
static void validateOptionalDir(const std::string &label, const std::string &path)
{
        if (!validateDirPath(path))
                throw std::runtime_error("Location validation error: " + label + " '" + path + "' is not a valid directory path.");
}

void  Location::validateLocation() const {
        // 1. Path must not be empty
        if (_path.empty())
                throw std::runtime_error("Location validation error: path is empty.");

        // 2. root and alias must not be used together
        if (!_root.empty() && !_alias.empty())
                throw std::runtime_error("Location validation error: cannot use both root and alias in the same location block.");

        // 4. If upload_path is relative, ensure root exists to resolve it
        if (!_upload_path.empty() && _upload_path[0] != '/') {
                if (_root.empty())
                        throw std::runtime_error("Location validation error: upload_path is relative but root is not set.");
        }

        // 5. Ensure at least one way to handle the request
        bool has_handler =
                (!_root.empty() || !_alias.empty()) ||          // static file serving
                (!_cgi_ext.empty() && !_cgi_path.empty());    // CGI handler

        if (!has_handler)
                throw std::runtime_error("Location validation error: no valid handling strategy defined (no root, alias, return, cgi, or upload).");

        // 6. Validate HTTP methods
        std::set<std::string>::const_iterator mit = _methods.begin();
        for (; mit != _methods.end(); ++mit) {
                if (*mit != "GET" && *mit != "POST" && *mit != "DELETE"  && *mit != "PUT")
                        throw std::runtime_error("Location validation error: invalid HTTP method '" + *mit + "'");
        }

        // 7. Validate error_page codes
        std::map<int, std::string>::const_iterator eit = _error_pages.begin();
        for (; eit != _error_pages.end(); ++eit) {
                if (eit->first < 400 || eit->first > 599)
                        throw std::runtime_error("Location validation error: invalid error_page code: " + to_string(eit->first));
                if (eit->second.empty())
                        throw std::runtime_error("Location validation error: error_page for code " + to_string(eit->first) + " has empty path.");
        }

        //8. Validate path
        if (!_root.empty())
                validateOptionalDir("root", _root);
        if (!_alias.empty())
                validateOptionalDir("alias", _alias);
        if (!_upload_path.empty()) {
                validateOptionalDir("upload_path", _upload_path);
                if (access(_upload_path.c_str(), R_OK | X_OK | W_OK) == 0)
                        throw std::runtime_error("Location validation error: upload_path '" + _upload_path + "' must be readable, writable, and executable.");
        }

        // Optional: validate CGI ext format (should start with '.')
        for (size_t i = 0; i < _cgi_ext.size(); ++i) {
                if (_cgi_ext[i].empty() || _cgi_ext[i][0] != '.')
                        throw std::runtime_error("Location validation error: CGI extension '" + _cgi_ext[i] + "' must start with a dot.");
        }
}


void Location::printDebug() const {
        std::cout << "=== Location Debug Info ===" << std::endl;

        std::cout << "Path: " << _path << std::endl;
        std::cout << "Root: " << _root << std::endl;
        std::cout << "Alias: " << (_alias.empty() ? "(none)" : _alias) << std::endl;
        std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
        std::cout << "Max Body Size: " << _client_max_body_size << std::endl;

        // Upload
        std::cout << "Upload Path: " << (_upload_path.empty() ? "(none)" : _upload_path) << std::endl;

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

        // CGI Paths
        std::cout << "CGI Paths: ";
        if (_cgi_path.empty()) {
                std::cout << "(none)";
        } else {
                for (size_t i = 0; i < _cgi_path.size(); ++i) {
                        std::cout << _cgi_path[i];
                        if (i < _cgi_path.size() - 1)
                                std::cout << ", ";
                }
        }
        std::cout << std::endl;

        // CGI Extensions
        std::cout << "CGI Extensions: ";
        if (_cgi_ext.empty()) {
                std::cout << "(none)";
        } else {
                for (size_t i = 0; i < _cgi_ext.size(); ++i) {
                        std::cout << _cgi_ext[i];
                        if (i < _cgi_ext.size() - 1)
                                std::cout << ", ";
                }
        }
        std::cout << std::endl;

        // Error Pages
        std::cout << "Error Pages: ";
        if (_error_pages.empty()) {
                std::cout << "(none)";
        } else {
                std::map<int, std::string>::const_iterator it = _error_pages.begin();
                for (; it != _error_pages.end(); ++it) {
                        std::cout << it->first << " -> " << it->second;
                        std::map<int, std::string>::const_iterator next = it;
                        ++next;
                        if (next != _error_pages.end())
                                std::cout << ", ";
                }
        }
        std::cout << std::endl;
        std::cout << "===========================" << std::endl;
}
