#include "../include/MIME.hpp"
#include <algorithm>

std::string MIME::get_type(const std::string &path) {
        static const std::map<std::string, std::string> mime_map = {
                {".html", "text/html"},
                {".htm", "text/html"},
                {".css", "text/css"},
                {".js", "application/javascript"},
                {".png", "image/png"},
                {".jpg", "image/jpeg"},
                {".jpeg", "image/jpeg"},
                {".gif", "image/gif"},
                {".svg", "image/svg+xml"},
                {".json", "application/json"},
                {".pdf", "application/pdf"},
                {".txt", "text/plain"},
                {".xml", "application/xml"}
        };

        std::string::size_type dot = path.find_last_of('.');
        if (dot == std::string::npos)
                return "application/octet-stream";

        std::string ext = path.substr(dot);
        for (size_t i = 0; i < ext.size(); ++i) {
                ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));
        }
        std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
        if (it != mime_map.end())
                return it->second;

        return "application/octet-stream";
}
