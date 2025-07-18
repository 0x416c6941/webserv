#!/bin/bash

# Read the POST data from stdin
read POST_DATA

# Extract and decode 'text' from form data
TEXT=$(echo "$POST_DATA" | sed -n 's/^text=//p' | sed 's/+/ /g' | sed 's/%20/ /g' | sed 's/&.*//')
TEXT=$(printf '%b' "${TEXT//%/\\x}")

# Append to target file
TARGET_FILE="data/html/trash/append_text.txt"
echo "$TEXT" >> "$TARGET_FILE"

# Output CGI-compliant HTTP response
echo "Content-Type: text/html"
echo
echo "<!DOCTYPE html>"
echo "<html><head><title>Append Result</title></head><body>"
echo "<h1>Text Appended</h1>"
echo "<p><strong>Appended Text:</strong> $(printf '%s' "$TEXT" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')</p>"
echo "<p><a href=\"/test_post.html\">⬅️ Back</a></p>"
echo "</body></html>"
