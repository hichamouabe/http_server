#include "Server.hpp"

Client::Client()
	: fd(-1),
	listen_fd(-1),
	state(READ_REQUEST_LINE),
	is_chunked(false),
	content_length(0),
	error_code(0),
	file_size(0),
	bytes_sent(0),
	header_sent(false),
	keep_alive(false),
	cgi_fd(-1),
	cgi_pid(-1)
{}

Client::Client(int fd)
	: fd(fd),
	listen_fd(-1),
	state(READ_REQUEST_LINE),
	is_chunked(false),
	content_length(0),
	error_code(0),
	file_size(0),
	bytes_sent(0),
	header_sent(false),
	keep_alive(false),
	connection_start_time(std::time(NULL)),
	last_activity_time(std::time(NULL)),
	cgi_fd(-1),
	cgi_pid(-1)
{}

Client::~Client() {}

// reset all per-request fields for keep-alive reuse

void	Client::reset() {
	recv_buf.clear();
	send_buf.clear();
	state	= READ_REQUEST_LINE;
	is_chunked = false;
	method.clear();
	path.clear();
	version.clear();
	header.clear();
	body.clear();
	content_length = 0;
	error_code	= 0;
	file_size	= 0;
	bytes_sent	= 0;
	header_sent	= false;
	cgi_fd 		= -1;
	cgi_pid		= -1;
	if (file_stream.is_open())
		file_stream.close();
}

// state and buffer

int	Client::getFd()	const {return fd;}
int	Client::getListenFd() const {return listen_fd;}
void	Client::setListenFd(int lfd) {listen_fd = lfd;}
std::string&	Client::recvBuf()	{return recv_buf;}
std::string&	Client::sendBuf()	{return send_buf;}
void	Client::setState(State s)	{state = s;}
State	Client::getState() const	{return state;}
bool	Client::requestComplete() const {
	return (recv_buf.find("\r\n\r\n") != std::string::npos);
}

// request setters

void	Client::setMethod(std::string m)		{method = m;}
void	Client::setPath(std::string p)			{path = p;}
void	Client::setVersion(std::string v)		{version = v;}
void	Client::setHeader(std::string key, std::string value)	{header[key] = value; }
void	Client::setBody(const std::string& b)		{body = b;}
void	Client::setContentLength(size_t cl)		{content_length = cl;}
void	Client::setErrorCode(int code)			{error_code = code;}
void	Client::setKeepAlive(bool ka)			{keep_alive = ka;}


// request getters

std::string                         Client::getMethod()        const { return method; }
std::string                         Client::getPath()          const { return path; }
std::string                         Client::getVersion()       const { return version; }
std::map<std::string, std::string>  Client::getHeader()        const { return header; }
std::string                         Client::getBody()          const { return body; }
size_t                              Client::getContentLength() const { return content_length; }
int                                 Client::getErrorCode()     const { return error_code; }
bool                                Client::isKeepAlive()      const { return keep_alive; }


// response file streaming

void	Client::openFile(const std::string& path) {
	file_stream.open(path.c_str(), std::ios::binary);
}

std::streamsize	Client::readFile(char* buf, std::size_t size) {
	if (!file_stream.is_open()) return -1;
	file_stream.read(buf, size);
	return file_stream.gcount();
}

void    Client::setFileSize(size_t fs)  { file_size = fs; }
void    Client::setHeaderSent(bool hs) { header_sent = hs; }
bool    Client::headerSent()           { return header_sent; }
void    Client::setBytesSent(size_t n) { bytes_sent += n; }
size_t  Client::getBytesSent()   const { return bytes_sent; }
size_t  Client::getFileSize()    const { return file_size; }

//////////////////////////////////////// time out functions
void Client::updateActivityTime() {
	last_activity_time = std::time(NULL);
}

time_t Client::getLastActivityTime() const {
	return last_activity_time;
}

bool Client::isInactiveFor(int seconds) const {
	return (std::time(NULL) - last_activity_time) > seconds;
}

int Client::getTimeoutForState() const {
	switch(state) {
		case READ_REQUEST_LINE:   return 5;    // 5 sec to send first line
		case READ_REQUEST_HEADER: return 10;   // 10 sec for headers
		case READ_BODY:           return 30;   // 30 sec for body upload
		case PROCESS_REQUEST:     return 60;   // 60 sec to process
		case PROCESS_CGI:	  return 5;
		case WRITE_RESPONSE:      return 30;   // 30 sec to send response
		case CLOSED:              return 0;
		default:                  return 30;
	}
}
void Client::setIsChunked(bool c) { is_chunked = c; }
bool Client::getIsChunked() const { return is_chunked; }
/////////////////////////////////////////////////////////end
