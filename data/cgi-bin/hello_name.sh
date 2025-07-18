#!/bin/sh

# What we just received from the client.
RESPONSE_BODY="<html><body>$(cat)</body></html>\n"

echo -ne "HTTP/1.1 200 OK\r\n"
echo -ne "Connection: close\r\n"
echo -ne "Content-Length: ${#RESPONSE_BODY}\r\n"
echo -ne "Content-Type: text/html\r\n"
echo -ne "Server: $SERVER_NAME\r\n"
echo -ne "\r\n"
echo -ne "$RESPONSE_BODY"
