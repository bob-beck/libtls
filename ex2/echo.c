#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CONNECTIONS 256
#define BUFLEN 4096

static int debug = 0;

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s host portnumber\n", __progname);
	exit(1);
}

#define STATE_READING 0
#define STATE_WRITING 1

struct client {
	int state;
	unsigned char *readptr, *writeptr;
	unsigned char buf[BUFLEN];
};

static struct client clients[MAX_CONNECTIONS];
static struct pollfd pollfds[MAX_CONNECTIONS];
static int throttle = 0;

static void
client_init(struct client *client)
{
	client->readptr = client->writeptr = client->buf;
	client->state = STATE_READING;
}

static ssize_t
client_get(struct client * client, unsigned char *outbuf, size_t outlen)
{
        unsigned char *nextptr = client->readptr;
        size_t n = 0;

        while (n < outlen) {
                if (nextptr == client->writeptr)
                        break;
                *outbuf++ = *nextptr++;
                if ((size_t)(nextptr - client->buf) >= sizeof(client->buf))
                        nextptr = client->buf;
                client->readptr = nextptr;
                n++;
        }

        if (debug && n > 0)
                fprintf(stderr, "client_get: got %ld bytes from buffer\n", n);

        return ((ssize_t)n);
}

static ssize_t
client_put(struct client *client, const unsigned char *inbuf, size_t inlen)
{
        unsigned char *nextptr = client->writeptr;
        unsigned char *prevptr;
        size_t n = 0;

        while (n < inlen) {
                prevptr = nextptr++;
                if ((size_t)(nextptr - client->buf) >= sizeof(client->buf))
                        nextptr = client->buf;
                if (nextptr == client->readptr)
                        break;
                *prevptr = *inbuf++;
                client->writeptr = nextptr;
                n++;
        }

        if (debug && n > 0)
                fprintf(stderr, "client_put: put %ld bytes into buffer\n", n);

        return ((ssize_t)n);
}

static void
closeconn (struct pollfd *pfd)
{
	close(pfd->fd);
	pfd->fd = -1;
	pfd->revents = 0;
	throttle = 0;
}

static void
newconn(struct pollfd *pfd, int newfd) {
	int sflags;
	if ((sflags = fcntl(newfd, F_GETFL)) < 0)
		err(1, "fcntl failed");
	sflags |= O_NONBLOCK;
	if (fcntl(newfd, F_SETFL, sflags) < 0)
		err(1, "fcntl failed");
	pfd->fd = newfd;
	pfd->events = POLLIN | POLLHUP;
	pfd->revents = 0;
}

static void
handle_client(struct pollfd *pfd, struct client *client)
{
	if ((pfd->revents & (POLLERR | POLLNVAL)))
		errx(1, "bad fd %d", pfd->fd);
	if (pfd->revents & POLLHUP)
		closeconn(pfd);
	else if (pfd->revents & pfd->events) {
		char buf[BUFLEN];
		ssize_t len = 0;
		if (client->state == STATE_READING) {
			len = read(pfd->fd, buf, sizeof(buf));
			if (len > 0) {
				if (client_put(client, buf, len)
				    != len) {
					warnx("client buffer failed");
					closeconn(pfd);
				} else {
					client->state=STATE_WRITING;
					pfd->events = POLLOUT | POLLHUP;
				}
			}
			else if (len == 0)
				closeconn(pfd);
			else
				pfd->events = POLLIN | POLLHUP;
		} else if (client->state == STATE_WRITING) {
			ssize_t w = 0;
			ssize_t written = 0;
			len = client_get(client, buf, sizeof(buf));
			do {
				w = write(pfd->fd, buf, len);
				if (w == -1) {
					if (errno != EINTR)
						closeconn(pfd);
				}
				else
					written += w;
			} while (written < len);
			if (pfd->fd > 0) {
				client->state = STATE_READING;
				pfd->events = POLLIN | POLLHUP;
			}
		}
	}
}

int main(int argc, char **argv) {

	struct addrinfo hints, *res;
	int i, listenfd, error;

	if (argc != 3)
		usage();

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((error = getaddrinfo(argv[1], argv[2], &hints, &res))) {
		fprintf(stderr, "%s\n", gai_strerror(error));
		usage();
	}

	for (i = 0; i < MAX_CONNECTIONS; i++)  {
		pollfds[i].fd = -1;
		pollfds[i].events = POLLIN | POLLHUP;
		pollfds[i].revents = 0;
	}

	if ((listenfd = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol)) < 0)
		err(1, "Couldn't get listen socket");

	if (bind(listenfd, res->ai_addr, res->ai_addrlen) == -1)
		err(1, "bind failed");

	if (listen(listenfd, MAX_CONNECTIONS) == -1)
		err(1, "listen failed");

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) == -1)
		err(1, "SO_REUSEADDR setsockopt failed");

	newconn(&pollfds[0], listenfd);

	while(1) {
		if (!throttle)
			pollfds[0].events = POLLIN | POLLHUP;
		else
			pollfds[0].events = 0;

		if (poll(pollfds, MAX_CONNECTIONS, -1) == -1)
			err(1, "poll failed");
		if (pollfds[0].revents) {
			struct sockaddr csaddr;
			socklen_t cssize;
			int fd;

			fd = accept(pollfds[0].fd, &csaddr, &cssize);
			throttle = 1;
			for (i = 1; fd >= 0 && i < MAX_CONNECTIONS; i++)  {
				if (pollfds[i].fd == -1) {
					newconn(&pollfds[i], fd);
					client_init(&clients[i]);
					throttle = 0;
					break;
				}
			}
		}
		for (i = 1; i < MAX_CONNECTIONS; i++)
			handle_client(&pollfds[i], &clients[i]);
	}

	freeaddrinfo(res);
	return 0;
}
