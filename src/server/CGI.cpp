#include "CGI.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
CGI::CGI() : _http_status(200) {}
CGI::~CGI() {}

void CGI::setMethod(const std::string& m) { _method = m; }
void CGI::setPath(const std::string& p) { _path = p; }
void CGI::setQuery(const std::string& q) { _query = q; }
void CGI::setBody(const std::string& b) { _body_in = b; }
void CGI::setContentType(const std::string& ct) { _content_type = ct; }
void CGI::setHost(const std::string& h) { _host = h; }

int CGI::executeAsync(const std::string& script, pid_t& out_pid) {
    int pipe_out[2];
    int pipe_in[2];

    if (pipe(pipe_out) < 0 || pipe(pipe_in) < 0) {
        std::cerr << "[CGI] ERROR: Pipe creation failed\n";
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[CGI] ERROR: Fork failed\n";
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_in[0]);  close(pipe_in[1]);
        return -1;
    }

    if (pid == 0) {
        // --- CHILD PROCESS ---
        close(pipe_out[0]);
        close(pipe_in[1]);

        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_in[0], STDIN_FILENO);

        close(pipe_out[1]);
        close(pipe_in[0]);

        // Build environment
        std::string env_method = "REQUEST_METHOD=" + _method;
        std::string env_query = "QUERY_STRING=" + _query;
        std::string env_content_type = "CONTENT_TYPE=" + _content_type;
        std::ostringstream content_len_oss;
        content_len_oss << _body_in.size();
        std::string env_content_length = "CONTENT_LENGTH=" + content_len_oss.str();
        std::string env_script = "SCRIPT_FILENAME=" + _path;
        std::string env_gateway = "GATEWAY_INTERFACE=CGI/1.1";
        std::string env_protocol = "SERVER_PROTOCOL=HTTP/1.1";
        std::string env_redirect = "REDIRECT_STATUS=200";

        char *env[] = {
            (char *)env_method.c_str(), (char *)env_query.c_str(),
            (char *)env_content_type.c_str(), (char *)env_content_length.c_str(),
            (char *)env_script.c_str(), (char *)env_gateway.c_str(),
            (char *)env_protocol.c_str(), (char *)env_redirect.c_str(), NULL
        };

        const char *args[] = { script.c_str(), _path.c_str(), NULL };
        execve(script.c_str(), (char * const *)args, env);
        
        std::cerr << "[CGI-CHILD] execve failed: " << strerror(errno) << std::endl;
        exit(127);
    }

    // --- PARENT PROCESS ---
    close(pipe_out[1]);
    close(pipe_in[0]);

    // Write POST body to CGI
    if (_method == "POST" && !_body_in.empty()) {
        write(pipe_in[1], _body_in.c_str(), _body_in.size());
    }
    close(pipe_in[1]); // Close immediately so CGI knows body is finished

    // Set reading pipe to Non-Blocking for epoll
    int flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

    out_pid = pid;
    return pipe_out[0]; // Return the FD so Server can add it to epoll
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
