#if !defined(__linux__) || !defined(__linux)
#error "Tiny is only buildable on Linux."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include <tiny.h>

#include <sys/epoll.h>

#define PORT ("80")
#define BACKLOG (64)
#define MAX_EPOLL_CLIENTS (64)

static int lsocket;

static void get_lsocket(void);
static void close_lsocket(void);

static void get_lsocket(void)
{
	struct addrinfo hint;
	struct addrinfo *addr_ll;
	struct addrinfo *idx;
	int gai_error;
	int yes = 1;

	(void) memset(&hint, 0, sizeof(hint));
	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	if ((gai_error = getaddrinfo(NULL, PORT, &hint, &addr_ll)) < 0) {
		fprintf(stderr, ERR "get_lsocket():getaddrinfo(): %s\n", gai_strerror(gai_error));
		exit(EXIT_FAILURE);
	}

	for (idx = addr_ll; idx != NULL; idx = idx->ai_next) {
		if ((lsocket=socket(idx->ai_family, idx->ai_socktype, idx->ai_protocol)) < 0) {
			perror(ERR "get_lsocket():socket()");
			continue;
		}

		if (setsockopt(lsocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			perror(ERR "get_lsocket():setsockopt()");
			close(lsocket);
			continue;
		}
		if (bind(lsocket, idx->ai_addr, idx->ai_addrlen) < 0) {
			perror(ERR "get_lsocket():bind()");
			close(lsocket);
			continue;
		}

		break;
	}

	if (idx == NULL) {
		goto failure;
	}

	atexit(close_lsocket);

	if (listen(lsocket, BACKLOG) < 0) {
		perror(ERR "get_lsocket():listen()");
		goto failure;
	}

	return ;

failure:
	fprintf(stderr, ERR "failed to create socket.\n");
	fprintf(stderr, "exiting...\n");
	exit(EXIT_FAILURE);
}

static void close_lsocket(void)
{
	close(lsocket);
}

int main()
{
	struct sockaddr client_data;
	socklen_t client_data_size = sizeof(client_data);

	struct epoll_event event;
	struct epoll_event event_list[MAX_EPOLL_CLIENTS];

	int epoll_fd;
	int client_socket;
	int active_fds;

	if (getuid() != 0) {
		fprintf(stderr, WARN "you may need to be root.\n");
	}

#if ENABLE_DEBUG
	fprintf(stderr, WARN "enabling debug can hurt performance.\n");
#endif

	get_lsocket();

	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		perror(ERR "main():epoll_create1()");
		exit(EXIT_FAILURE);
	}

	event.events = EPOLLIN;
	event.data.fd = lsocket;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, lsocket, &event) < 0) {
		perror(ERR "main():epoll_ctl()");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, STAT "starting main loop...\n\n");
	for (;;) {
		active_fds = epoll_wait(epoll_fd, event_list, MAX_EPOLL_CLIENTS, -1);
		if (active_fds < 0) {
			perror(ERR "main() loop:epoll_wait()");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < active_fds; i++) {
			if (event_list[i].data.fd == lsocket) {
				client_socket = accept(lsocket, &client_data, &client_data_size);
				if (UNLIKELY(client_socket < 0)) {
					perror(ERR "main() loop:accept()");
					continue;
				}

				int ret = fcntl(client_socket, F_SETFL, fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);
				if (UNLIKELY(ret < 0)) {
					perror(ERR "main() loop:fcntl()");
					continue;
				}

				event.events = EPOLLIN | EPOLLET;
				event.data.fd = client_socket;
				if (UNLIKELY(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) < 0)) {
					perror(ERR "main() loop:epoll_ctl()");
					continue;
				}
			} else {
				DEBUG("request is ready, handling it.\n");
				handle_request(event_list[i].data.fd);
				close(event_list[i].data.fd);
				DEBUG("closed request socket.\n\n");
			}
		}
	}
	exit(EXIT_SUCCESS);
}

