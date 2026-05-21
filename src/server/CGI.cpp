#include "CGI.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
#include <cstdio>  
#include <ctime>  
#include <unistd.h>

CGI::CGI() : _http_status(200) {}
CGI::~CGI() {}

void CGI::setMethod(const std::string& m) { _method = m; }
void CGI::setPath(const std::string& p) { _path = p; }
void CGI::setQuery(const std::string& q) { _query = q; }
void CGI::setBody(const std::string& b) { _body_in = b; }
void CGI::setContentType(const std::string& ct) { _content_type = ct; }
void CGI::setHost(const std::string& h) { _host = h; }


void CGI::setHeaders(const std::map<std::string, std::string>& headers) {
    _client_headers = headers;
}

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

        for (std::map<std::string, std::string>::const_iterator it = _client_headers.begin(); it != _client_headers.end(); ++it) {
            std::string key = it->first;
            std::string env_key = "HTTP_";
            for (size_t i = 0; i < key.length(); ++i) {
                if (key[i] == '-') env_key += '_';
                else env_key += std::toupper(key[i]);
            }
            env_strings.push_back(env_key + "=" + it->second);
        }

        std::vector<char*> env_ptrs;
        for (size_t i = 0; i < env_strings.size(); ++i) {
            env_ptrs.push_back(const_cast<char*>(env_strings[i].c_str()));
        }
        env_ptrs.push_back(NULL);

        const char *args[] = { script.c_str(), exec_path.c_str(), NULL };

        execve(script.c_str(), (char * const *)args, &env_ptrs[0]);

        std::cerr << "[CGI-CHILD] execve failed: " << strerror(errno) << std::endl;
        exit(127);
    }
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
