#include "Server.hpp"

Client::Client()
	: fd(-1),
	listen_fd(-1),
	state(READ_REQUEST_LINE),
	content_length(0),
	error_code(0),
	file_size(0),
	bytes_sent(0),
	header_sent(false),
	keep_alive(false)
{}

Client::Client(int fd)
	: fd(fd),
	listen_fd(-1),
	state(READ_REQUEST_LINE),
	content_length(0),
	error_code(0),
	file_size(0),
	bytes_sent(0),
	header_sent(false),
	keep_alive(false)
{}

Client::~Client() {}

// reset all per-request fields for keep-alive reuse

void	Client::reset() {
	recv_buf.clear();
	send_buf.clear();
	state	= READ_REQUEST_LINE;
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


