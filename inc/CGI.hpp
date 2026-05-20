#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <vector> 
class CGI {
    private:
        std::string _method;
        std::string _path;
        std::string _query;
        std::string _body_in;
        std::string _content_type;
        std::string _host;
	std::map<std::string, std::string> _client_headers;	
        std::string _headers_out;
        std::string _body_out;
        int         _http_status;
        
    public:
        CGI();
        ~CGI();
        
        void setMethod(const std::string& m);
        void setPath(const std::string& p);
        void setQuery(const std::string& q);
        void setBody(const std::string& b);
        void setContentType(const std::string& ct);
        void setHost(const std::string& h);
        void setHeaders(const std::map<std::string, std::string>& headers);
        // Starts the process and returns the pipe FD to read from. Sets pid by reference.
        int  executeAsync(const std::string& script, pid_t& out_pid);
        
        // Parses the raw data collected by epoll
        void parseOutput(const std::string& raw_output);
        
        std::string getHeaders() const;
        std::string getBody() const;
        int         getStatus() const;
};

#endif
