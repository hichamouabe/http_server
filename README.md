This project has been created as part of the 42 curriculum by houabell & achemlal

Webserv
This is when you finally understand why URLs start with HTTP

Description
Webserv is a custom HTTP server written from scratch in C++98. The goal of this project is to understand the inner workings of the HTTP protocol, non-blocking I/O operations, and client-server network communication.

While heavily inspired by HTTP/1.1, this project implements a pragmatic subset of the protocol. It balances strict adherence to the RFCs with practical implementation based on Postel's Law ("be conservative in what you do, be liberal in what you accept from others"), much like modern real-world servers.

Instead of relying on heavy frameworks or existing web servers like NGINX or Apache, this project implements the core mechanics of an HTTP server. It leverages I/O multiplexing (epoll) to handle multiple concurrent client connections efficiently, reads a custom NGINX-like configuration file, and manages routing, file uploads, and dynamic CGI execution.

Key Features
Non-blocking I/O: Fully asynchronous socket operations using a single epoll instance.
HTTP Methods: Support for GET, POST, and DELETE requests.
Custom Configuration: Parses an NGINX-style .conf file to configure routes, ports, max body sizes, and default pages.
Virtual Hosting: Hosts multiple websites on the same IP/Port using the server_name directive (Host header resolution).
CGI Support: Executes dynamic scripts via the Common Gateway Interface (CGI), parsing environment variables and piping standard I/O asynchronously.
File Management: Supports downloading static files, auto-indexing directories, and handling file uploads.
Robustness: Implements an internal state machine to prevent hanging connections, handle timeouts, and manage large requests safely.
Instructions
Compilation
The project includes a Makefile that complies with the 42 standard. To compile the server, simply run:

make
Other available rules:

make clean: Removes object files.
make fclean: Removes object files and the executable.
make re: Recompiles the entire project.
Execution
Run the server by providing a configuration file as an argument. If no argument is provided, it will attempt to load a default configuration file.

./webserv [path/to/config.conf]
Example:

./webserv configs/status.conf
Installation Prerequisites
OS: Linux (Uses epoll for multiplexing).
Compiler: c++ (Clang or GCC) with C++98 support.
CGI Binaries: To test CGI scripts, ensure the respective binaries are installed on your system (e.g., apt install php-cgi python3).
Usage Examples & Test Cases
Once the server is running (e.g., on localhost:8080), you can test the various HTTP status codes and features using curl:

1. 200 OK (Standard GET request)

curl -i http://localhost:8080/
2. 201 Created (Upload a file) (Note: Requires the Content-Disposition header based on our implementation)

curl -i -X POST -H 'Content-Disposition: attachment; filename="test.txt"' -d "File contents" http://localhost:8080/upload
3. 204 No Content (Delete the file we just uploaded)

curl -i -X DELETE http://localhost:8080/upload/test.txt
4. 301 Moved Permanently (Test the 'return' redirection directive)

curl -i http://localhost:8080/redirect
5. 403 Forbidden (Access a directory where autoindex is off)

curl -i http://localhost:8080/forbidden/
6. 404 Not Found (Request a file that does not exist)

curl -i http://localhost:8080/does_not_exist.html
7. 405 Method Not Allowed (Send POST to root, which only allows GET)

curl -i -X POST http://localhost:8080/
8. 413 Content Too Large (Send a body exceeding the size limit of a specific location)

curl -i -X POST -d "This string is much longer than 5 bytes" http://localhost:8080/small
9. 431 Request Header Fields Too Large (Generates and sends a massive header)

curl -i -H "X-Massive-Header: $(head -c 9000 < /dev/zero | tr '\0' 'A')" http://localhost:8080/
10. 500 Internal Server Error (Attempt to save upload to a non-existent physical directory)

curl -i -X POST -d "Data" http://localhost:8080/bad_upload/fail.txt
Technical Choices
State Machine: The server uses an event-driven state machine (READ_REQUEST_LINE, READ_REQUEST_HEADER, READ_BODY, PROCESS_REQUEST, PROCESS_CGI, WRITE_RESPONSE). This ensures the server never blocks while waiting for partial data from a client.
I/O Multiplexing: epoll was chosen as the underlying polling mechanism to achieve high performance and O(1) file descriptor event monitoring.
CGI Execution: CGI processes are executed asynchronously using fork() and execve(). Bi-directional pipes are used in non-blocking mode to pass request bodies and read script outputs without halting the main server loop.
Resources
References & Documentation:

RFC 7230 - HTTP/1.1: Message Syntax and Routing
RFC 3875 - The Common Gateway Interface (CGI) Version 1.1
AI Usage Statement: During the development of this project, Artificial Intelligence (Large Language Models) was used as a learning, debugging, and validation aid, adhering to the pedagogical guidelines of the 42 curriculum. Specifically, AI was used to:

Debugging & Validation: Help identify bugs, validate code cleanliness, and ensure logic was implemented efficiently (avoiding "stupid" or overly complex approaches).
Concept Explanation & Security: Explain dense networking concepts and provide advice on security tricks during development (e.g., sanitizing inputs and preventing path traversal attacks).
RFC Navigation: Guide us through the massive HTTP RFCs, helping to distinguish between strict RFC requirements and pragmatic real-world implementations based on Postel's Law (acting conservatively in what we send, and liberally in what we accept). All code decisions and logic were thoroughly understood, manually implemented, and rigorously peer-reviewed to ensure complete ownership of the project.

