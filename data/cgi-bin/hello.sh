#!/bin/sh

RESPONSE_BODY="<html><body>Hello from shell!</body></html>"

printf "HTTP/1.1 200 OK\r\n"
printf "Connection: close\r\n"
printf "Content-Length: ${#RESPONSE_BODY}\r\n"
printf "Content-Type: text/html\r\n"
printf "Server: $SERVER_NAME\r\n"
printf "\r\n"
printf "${RESPONSE_BODY}"
