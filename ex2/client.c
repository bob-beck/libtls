/*
 * Copyright (c) 2018 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * A relatively simple buffering echo client that uses poll(2),
 * for instructional purposes.
 */

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
#include <tls.h>
#include <unistd.h>


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
#define STATE_NONE 2

struct server {
	int state;
	unsigned char *readptr, *writeptr, *nextptr;
	unsigned char buf[BUFLEN];
};

static struct server server;

static void
server_init(struct server *server)
{
	server->readptr = server->writeptr = server->nextptr = server->buf;
	server->state = STATE_NONE;
}

static void
server_consume(struct server *server)
{
	server->readptr = server->nextptr;
}

static ssize_t
server_get(struct server * server, unsigned char *outbuf, size_t outlen)
{
        unsigned char *nextptr = server->readptr;
        size_t n = 0;

        while (n < outlen) {
                if (nextptr == server->writeptr)
                        break;
                *outbuf++ = *nextptr++;
                if ((size_t)(nextptr - server->buf) >= sizeof(server->buf))
                        nextptr = server->buf;
		server->nextptr = nextptr;
                n++;
        }

        if (debug && n > 0)
                fprintf(stderr, "server_get: got %ld bytes from buffer\n", n);

        return ((ssize_t)n);
}

static ssize_t
server_put(struct server *server, const unsigned char *inbuf, size_t inlen)
{
        unsigned char *nextptr = server->writeptr;
        unsigned char *prevptr;
        size_t n = 0;

        while (n < inlen) {
                prevptr = nextptr++;
                if ((size_t)(nextptr - server->buf) >= sizeof(server->buf))
                        nextptr = server->buf;
                if (nextptr == server->readptr)
                        break;
                *prevptr = *inbuf++;
                server->writeptr = nextptr;
                n++;
        }

        if (debug && n > 0)
                fprintf(stderr, "server_put: put %ld bytes into buffer\n", n);

        return ((ssize_t)n);
}

static void
closeconn (struct pollfd *pfd)
{
	close(pfd->fd);
	pfd->fd = -1;
	pfd->revents = 0;
	exit(0);
}

static void
newconn(struct pollfd *pfd, int newfd, int events) {
	int sflags;
	if ((sflags = fcntl(newfd, F_GETFL)) < 0)
		err(1, "fcntl failed");
	sflags |= O_NONBLOCK;
	if (fcntl(newfd, F_SETFL, sflags) < 0)
		err(1, "fcntl failed");
	pfd->fd = newfd;
	pfd->events = events;
	pfd->revents = 0;
}

static void
handle_server(struct pollfd *pfd, struct server *server)
{
	if ((pfd->revents & (POLLERR | POLLNVAL)))
		errx(1, "bad fd %d", pfd->fd);
	if (pfd->revents & POLLHUP)
		closeconn(pfd);
	else if (pfd->revents & pfd->events) {
		char buf[BUFLEN];
		ssize_t len = 0;
		if (server->state == STATE_READING) {
			ssize_t w = 0;
			ssize_t written = 0;
			len = read(pfd->fd, buf, sizeof(buf));
			if (len > 0) {
				do {
					w = write(STDOUT_FILENO, buf, len);
					if (w == -1) {
						if (errno != EINTR)
							closeconn(pfd);
					}
					else
						written += w;
				} while (written < len);
				if (buf[len - 1] == '\n') {
					server->state=STATE_NONE;
					pfd->events = POLLHUP;
				}
			}
			else if (len == 0)
				closeconn(pfd);
			else
				pfd->events = POLLIN | POLLHUP;
		} else if (server->state == STATE_WRITING) {
			ssize_t w = 0;
			ssize_t written = 0;
			len = server_get(server, buf, sizeof(buf));
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
				server_consume(server);
				server->state = STATE_READING;
				pfd->events = POLLIN | POLLHUP;
			}
		}
	}
}

int main(int argc, char **argv) {
	struct tls_config *tls_cfg = NULL;
        struct tls *tls_ctx = NULL;
	struct addrinfo hints, *res;
	int serverfd, error;
	struct pollfd pollfd;

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

        /* now set up TLS */
        if (tls_init() == -1)
                errx(1, "unable to initialize TLS");
        if ((tls_cfg = tls_config_new()) == NULL)
                errx(1, "unable to allocate TLS config");
        if (tls_config_set_ca_file(tls_cfg, "../CA/root.pem") == -1)
                errx(1, "unable to set root CA file");

	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket failed");

	if (connect(serverfd, res->ai_addr, res->ai_addrlen) == -1)
		err(1, "connect failed");

	if ((tls_ctx = tls_client()) == NULL)
                errx(1, "tls client creation failed");
        if (tls_configure(tls_ctx, tls_cfg) == -1)
                errx(1, "tls configuration failed (%s)",
                    tls_error(tls_ctx));
        if (tls_connect_socket(tls_ctx, serverfd, "localhost") == -1)
                errx(1, "tls connection failed (%s)",
                    tls_error(tls_ctx));

	newconn(&pollfd, serverfd, 0);
	server_init(&server);

	while(1) {
		if (server.state == STATE_NONE) {
			char *line = NULL;
			size_t size = 0;
			ssize_t len;

			if ((len = getline(&line, &size, stdin)) != -1) {
				if (server_put(&server, line, len) != len)
					errx(1, "can't buffer line to server");
				server.state=STATE_WRITING;
				pollfd.events = POLLOUT | POLLHUP;
			}
			free(line);
			if (len == -1)
				break;
		}
		if (poll(&pollfd, 1, -1) == -1)
			err(1, "poll failed");
		handle_server(&pollfd, &server);
	}

	freeaddrinfo(res);
	return 0;
}
