#!/bin/sh

echo "Content-Type: text/html"
echo ""

echo "<html><body>"
echo "<h1>POST received</h1>"
echo "<pre>"

# Read input data from stdin
cat

echo "</pre>"
echo "</body></html>"
