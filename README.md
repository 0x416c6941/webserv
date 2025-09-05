# webserv
PoC implementation of *HTTP 1.1* compilant web server in C++.
<br>
The main goal of this project is to introduce concepts
of web programming using **POSIX sockets** at *42 School*.
<br>
<br>
In our case, the following request types are implemented:
- GET
- POST
- DELETE
- PUT

## Building and usage
To build the project, simply type:
```
make
```

To run, use:
```
./webserv <config>
```

Or just simply:
```
./webserv
```
In the latter case a default config at **configs/default.conf** will be used.
<br>
To see all possible configuration options, please consult
with that same **configs/default.conf** configuration file.
