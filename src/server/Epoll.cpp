#include "Server.hpp"

// adding fd to epoll for read event (EPOLLIN)

void	Server::addToEpoll(int fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

// if we have a response ready and we have to send 

void	Server::modifyEpoll(int fd, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

// check wach fd one of our listening sockets (checkih wach wdina hh)

bool	Server::isListenFd(int fd) {
	return std::find(listenfd.begin(), listenfd.end(), fd) != listenfd.end();
}


// main loop , epoll_wait until one fd is ready 

/*void	Server::eventLoop() {
	const int MAX_EVENTS = 1024;
	struct epoll_event events[MAX_EVENTS];

	while (true) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (n <= 0) continue;

		for (int i = 0; i < n; ++i) {
			int fd = events[i].data.fd;
			// error or hangup clean up the client
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				disconnect(fd);
				continue;
			}
			if (isListenFd(fd)) {
				// new connection
				acceptClients(fd);
			}
			else {
				Client& c = *clients[fd];
				if (c.getState() < PROCESS_REQUEST)
					handleRequest(fd);
				if (c.getState() == WRITE_RESPONSE)
					handleResponse(fd);
				if (c.getState() == CLOSED)
					disconnect(fd);
			}
		}
	}
}*/

void Server::eventLoop() {
	const int MAX_EVENTS = 1024;
	struct epoll_event events[MAX_EVENTS];
	const int EPOLL_TIMEOUT_MS = 1000;  // 1 second tick

	// Track the last time we did a full sweep of the clients map
	time_t last_timeout_sweep = std::time(NULL);

	while (true) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);

		// ========== 1. PROCESS NETWORK EVENTS ==========
		for (int i = 0; i < n; ++i) {
			int fd = events[i].data.fd;

			// error or hangup clean up the client
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				disconnect(fd);
				continue;
			}
			if (isListenFd(fd)) {
				// new connection
				acceptClients(fd);
			}
			else {
				Client& c = *clients[fd];
				c.updateActivityTime();  // Client is active, reset timer

				if (c.getState() < PROCESS_REQUEST)
					handleRequest(fd);
				if ( c.getState() == WRITE_RESPONSE)
					handleResponse(fd);
				if (c.getState() == CLOSED)
					disconnect(fd);
			}
		}

		// ========== 2. TIMEOUT SWEEP (runs every 1 second) ==========
		time_t now = std::time(NULL);

		// Only run the O(N) map sweep once per second
		if (now - last_timeout_sweep >= 1) {
			std::vector<int> dead_clients;  // Store FDs to kill

			for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
				Client* c = it->second;
				int limit = c->getTimeoutForState();

				if (c->isInactiveFor(limit)) {
					std::cout << "[TIMEOUT] fd=" << it->first
					          << " state=" << c->getState()
					          << " inactive for " << (now - c->getLastActivityTime())
					          << "s (limit=" << limit << "s)" << std::endl;
					dead_clients.push_back(it->first);
				}
			}

			// Safely disconnect after the loop
			for (size_t i = 0; i < dead_clients.size(); ++i) {
				disconnect(dead_clients[i]);
			}

			last_timeout_sweep = now;
		}
	}
}
