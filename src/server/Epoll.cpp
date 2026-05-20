#include "Server.hpp"
#include <sys/wait.h>
#include <signal.h>

void	Server::addToEpoll(int fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}


void	Server::modifyEpoll(int fd, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}


bool	Server::isListenFd(int fd) {
	return std::find(listenfd.begin(), listenfd.end(), fd) != listenfd.end();
}
void Server::eventLoop() {
	const int MAX_EVENTS = 1024;
	struct epoll_event events[MAX_EVENTS];
	const int EPOLL_TIMEOUT_MS = 1000;  

	time_t last_timeout_sweep = std::time(NULL);

	while (true) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (cgi_clients.find(fd) != cgi_clients.end()) {
                handleCGIRead(fd);
                continue; 
            }

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                disconnect(fd);
                continue;
            }
            if (isListenFd(fd)) {
                acceptClients(fd);
            }
            else {
                Client& c = *clients[fd];
                c.updateActivityTime();  

                if (c.getState() < PROCESS_REQUEST)
                    handleRequest(fd);
                if (c.getState() == WRITE_RESPONSE)
                    handleResponse(fd);
                if (c.getState() == CLOSED)
                    disconnect(fd);
            }
        }
	time_t now = std::time(NULL);

	if (now - last_timeout_sweep >= 1) {
		std::vector<int> dead_clients;

		for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
			Client* c = it->second;
			int limit = c->getTimeoutForState();

			if (c->isInactiveFor(limit)) {
				if (c->getState() == PROCESS_CGI) {
					std::cout << "[TIMEOUT] CGI PID " << c->cgi_pid << " hung! Sending 504." << std::endl;
					
					kill(c->cgi_pid, SIGKILL);
					waitpid(c->cgi_pid, NULL, 0);
					
					epoll_ctl(epfd, EPOLL_CTL_DEL, c->cgi_fd, NULL);
					close(c->cgi_fd);
					cgi_clients.erase(c->cgi_fd);
					
					c->cgi_fd = -1;
					c->cgi_pid = -1;
					c->setErrorCode(504);
					c->setState(PROCESS_REQUEST);
					buildResponse(*c);
					c->setState(WRITE_RESPONSE);
					modifyEpoll(it->first, EPOLLOUT);
				} 
				else {
					std::cout << "[TIMEOUT] fd=" << it->first << " inactive. Disconnecting." << std::endl;
					dead_clients.push_back(it->first);
				}
			}
		}

		for (size_t i = 0; i < dead_clients.size(); ++i) {
			disconnect(dead_clients[i]);
		}

		last_timeout_sweep = now;
		}
	}
}
