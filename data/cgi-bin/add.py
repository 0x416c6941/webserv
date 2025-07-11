#!/usr/bin/env python3

import cgi

print("Content-Type: text/html\n")

form = cgi.FieldStorage()

try:
    a = int(form.getfirst("a", 0))
    b = int(form.getfirst("b", 0))
    result = a + b
    print(f"<html><body><h1>{a} + {b} = {result}</h1></body></html>")
except Exception as e:
    print(f"<html><body><h1>Error: {e}</h1></body></html>")
