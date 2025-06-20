#include "../include/Webserv.hpp"

std::string getReasonPhrase(size_t status_code) {
    switch (status_code) {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown Error";
    }
}

std::string generateErrorPage(size_t status_code) {
    std::string reason = getReasonPhrase(status_code);

    std::ostringstream body;
    body << "<!DOCTYPE html>\n"
         << "<html><head><title>" << status_code << " " << reason << "</title></head>\n"
         << "<body><h1>" << status_code << " " << reason << "</h1>\n"
         << "<p>The server encountered an error processing your request.</p>\n"
         << "</body></html>\n";

    std::string body_str = body.str();
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << reason << "\r\n"
             << "Content-Length: " << body_str.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body_str;

    return response.str();
}
