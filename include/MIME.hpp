#pragma once
#include "Webserv.hpp"

class MIME {
public:
    static std::string get_type(const std::string &path);
};
