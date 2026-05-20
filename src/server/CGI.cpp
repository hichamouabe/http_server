#include "CGI.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
#include <cstdio>  // For std::remove
#include <ctime>   // For time()
#include <unistd.h>

CGI::CGI() : _http_status(200) {}
CGI::~CGI() {}

void CGI::setMethod(const std::string& m) { _method = m; }
void CGI::setPath(const std::string& p) { _path = p; }
void CGI::setQuery(const std::string& q) { _query = q; }
void CGI::setBody(const std::string& b) { _body_in = b; }
void CGI::setContentType(const std::string& ct) { _content_type = ct; }
void CGI::setHost(const std::string& h) { _host = h; }

/*int CGI::executeAsync(const std::string& script, pid_t& out_pid) {
    int pipe_out[2];

    // We only need ONE pipe now: for Webserv to READ the CGI's output.
    if (pipe(pipe_out) < 0) {
        std::cerr << "[CGI] ERROR: Pipe creation failed\n";
        return -1;
    }

    // Generate a unique temporary filename
    std::ostringstream tmp_name;
    tmp_name << "/tmp/webserv_cgi_" << time(NULL) << "_" << rand();
    std::string tmp_file = tmp_name.str();

    // Write POST body to a regular disk file BEFORE forking.
    // (The evaluation sheet explicitly EXEMPTS disk files from the epoll requirement)
    if (_method == "POST" && !_body_in.empty()) {
        int fd_out = open(tmp_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_out >= 0) {
            write(fd_out, _body_in.c_str(), _body_in.size());
            close(fd_out);
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[CGI] ERROR: Fork failed\n";
        close(pipe_out[0]); 
        close(pipe_out[1]);
        std::remove(tmp_file.c_str());
        return -1;
    }

     if (pid == 0) {
        // --- CHILD PROCESS ---
        close(pipe_out[0]);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_out[1]);

// --- START FIX 1: Bulletproof STDIN Redirection ---
        int fd_in = -1;
        if (_method == "POST" && !_body_in.empty()) {
            fd_in = open(tmp_file.c_str(), O_RDONLY);
            if (fd_in >= 0) {
                std::remove(tmp_file.c_str());
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
        }
        
        // If it's a GET request, OR if the POST file failed to open, force /dev/null
        if (fd_in < 0) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        // --- END FIX 1 ---

        // --- THE FIX: Isolate the filename from the path ---
        std::string exec_path = _path;
        size_t last_slash = _path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir = _path.substr(0, last_slash);
            if (chdir(dir.c_str()) == -1) {
                exit(1);
            }
            // After changing directory, the script is just the filename in the current dir
            exec_path = _path.substr(last_slash + 1); 
        }

        // Build environment
        std::string env_method = "REQUEST_METHOD=" + _method;
        std::string env_query = "QUERY_STRING=" + _query;
        std::string env_content_type = "CONTENT_TYPE=" + _content_type;
        std::ostringstream content_len_oss;
        content_len_oss << _body_in.size();
        std::string env_content_length = "CONTENT_LENGTH=" + content_len_oss.str();
        
        // Pass the fixed exec_path to SCRIPT_FILENAME
        std::string env_script = "SCRIPT_FILENAME=" + exec_path;
        std::string env_gateway = "GATEWAY_INTERFACE=CGI/1.1";
        std::string env_protocol = "SERVER_PROTOCOL=HTTP/1.1";
        std::string env_redirect = "REDIRECT_STATUS=200";

        char *env[] = {
            (char *)env_method.c_str(), (char *)env_query.c_str(),
            (char *)env_content_type.c_str(), (char *)env_content_length.c_str(),
            (char *)env_script.c_str(), (char *)env_gateway.c_str(),
            (char *)env_protocol.c_str(), (char *)env_redirect.c_str(), NULL
        };

        // Pass the fixed exec_path to python/php
        const char *args[] = { script.c_str(), exec_path.c_str(), NULL };
        execve(script.c_str(), (char * const *)args, env);
        
        std::cerr << "[CGI-CHILD] execve failed: " << strerror(errno) << std::endl;
        exit(127);
    }
    // --- PARENT PROCESS ---
    close(pipe_out[1]);

    // Parent cleans up the tmp file just in case the fork failed or child didn't run
    //std::remove(tmp_file.c_str());

    // Set reading pipe to Non-Blocking for epoll
    int flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

    out_pid = pid;
    return pipe_out[0]; // Return the FD so Server can add it to epoll
}*/

void CGI::setHeaders(const std::map<std::string, std::string>& headers) {
    _client_headers = headers;
}

// UPDATE executeAsync
int CGI::executeAsync(const std::string& script, pid_t& out_pid) {
    int pipe_out[2];

    if (pipe(pipe_out) < 0) {
        std::cerr << "[CGI] ERROR: Pipe creation failed\n";
        return -1;
    }

    std::ostringstream tmp_name;
    tmp_name << "/tmp/webserv_cgi_" << time(NULL) << "_" << rand();
    std::string tmp_file = tmp_name.str();

    if (_method == "POST" && !_body_in.empty()) {
        int fd_out = open(tmp_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_out >= 0) {
            write(fd_out, _body_in.c_str(), _body_in.size());
            close(fd_out);
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[CGI] ERROR: Fork failed\n";
        close(pipe_out[0]);
        close(pipe_out[1]);
        std::remove(tmp_file.c_str());
        return -1;
    }

     if (pid == 0) {
        // --- CHILD PROCESS ---
        close(pipe_out[0]);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_out[1]);

        int fd_in = -1;
        if (_method == "POST" && !_body_in.empty()) {
            fd_in = open(tmp_file.c_str(), O_RDONLY);
            if (fd_in >= 0) {
                std::remove(tmp_file.c_str());
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
        }

        if (fd_in < 0) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }

        std::string exec_path = _path;
        size_t last_slash = _path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir = _path.substr(0, last_slash);
            if (chdir(dir.c_str()) == -1) {
                exit(1);
            }
            exec_path = _path.substr(last_slash + 1);
        }

        // =========================================================
        // --- NEW DYNAMIC ENVIRONMENT BUILDER (INCLUDES COOKIES) ---
        // =========================================================
        std::vector<std::string> env_strings;

        env_strings.push_back("REQUEST_METHOD=" + _method);
        env_strings.push_back("QUERY_STRING=" + _query);
        env_strings.push_back("CONTENT_TYPE=" + _content_type);

        std::ostringstream content_len_oss;
        content_len_oss << _body_in.size();
        env_strings.push_back("CONTENT_LENGTH=" + content_len_oss.str());

        env_strings.push_back("SCRIPT_FILENAME=" + exec_path);
        env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
        env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
        env_strings.push_back("REDIRECT_STATUS=200");

        // Convert client headers to HTTP_VAR_NAME
        for (std::map<std::string, std::string>::const_iterator it = _client_headers.begin(); it != _client_headers.end(); ++it) {
            std::string key = it->first;
            std::string env_key = "HTTP_";
            for (size_t i = 0; i < key.length(); ++i) {
                if (key[i] == '-') env_key += '_';
                else env_key += std::toupper(key[i]);
            }
            env_strings.push_back(env_key + "=" + it->second);
        }

        // Convert vector of strings into char* array for execve
        std::vector<char*> env_ptrs;
        for (size_t i = 0; i < env_strings.size(); ++i) {
            env_ptrs.push_back(const_cast<char*>(env_strings[i].c_str()));
        }
        env_ptrs.push_back(NULL);

        const char *args[] = { script.c_str(), exec_path.c_str(), NULL };

        // Use dynamically built environment array
        execve(script.c_str(), (char * const *)args, &env_ptrs[0]);

        std::cerr << "[CGI-CHILD] execve failed: " << strerror(errno) << std::endl;
        exit(127);
    }
    // --- PARENT PROCESS ---
    close(pipe_out[1]);

    int flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

    out_pid = pid;
    return pipe_out[0];
}

void CGI::parseOutput(const std::string& raw_output) {
    size_t sep = raw_output.find("\r\n\r\n");
    int sep_len = 4;

    if (sep == std::string::npos) {
        sep = raw_output.find("\n\n");
        sep_len = 2;
    }

    if (sep != std::string::npos) {
        _headers_out = raw_output.substr(0, sep);
        _body_out = raw_output.substr(sep + sep_len);
    } else {
        _headers_out = "";
        _body_out = raw_output;
    }

    // Default to 200, check if script provided a custom Status
    _http_status = 200;
    if (!_headers_out.empty()) {
        std::istringstream hstream(_headers_out);
        std::string line;
        while (std::getline(hstream, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);

            size_t colon = line.find(": ");
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                if (key == "Status") {
                    _http_status = std::atoi(line.substr(colon + 2).c_str());
                }
            }
        }
    }
}

std::string CGI::getHeaders() const { return _headers_out; }
std::string CGI::getBody() const { return _body_out; }
int CGI::getStatus() const { return _http_status; }
