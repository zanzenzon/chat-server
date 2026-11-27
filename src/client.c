#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

/* ------------------------------------------------------------------ client
 *
 * One text line per message (see common.h protocol).
 *   client -> server: JOIN <name> | MSG <text> | LIST | QUIT
 *
 * Two threads:
 *   - input thread : reads the keyboard and sends MSG/LIST/QUIT lines
 *   - recv thread  : loops on recv() and prints incoming lines
 *
 * The socket is shared; input closes/shuts it down on /quit, which wakes
 * the recv thread.  SIGPIPE is ignored so a write to a closed socket does
 * not kill the process.
 */

static int sock = -1;
static volatile int running = 1;
static pthread_mutex_t io_lock = PTHREAD_MUTEX_INITIALIZER;

static void safe_print(const char *msg)
{
	pthread_mutex_lock(&io_lock);
	fputs(msg, stdout);
	fflush(stdout);
	pthread_mutex_unlock(&io_lock);
}

/* recv thread: buffer bytes until a newline, then print the complete line. */
static void *recv_thread(void *arg)
{
	(void)arg;
	char buf[BUF_SIZE];
	char line[BUF_SIZE];
	size_t line_len = 0;
	ssize_t n;

	while (running) {
		n = recv(sock, buf, sizeof(buf) - 1, 0);
		if (n <= 0) {
			if (running)
				safe_print("\n[server] connection closed by server\n");
			break;
		}
		buf[n] = '\0';

		for (ssize_t i = 0; i < n; i++) {
			char c = buf[i];
			if (c == '\n') {
				line[line_len] = '\0';
				if (line_len > 0) {
					char out[BUF_SIZE + 16];
					snprintf(out, sizeof(out), "\n%s\n", line);
					safe_print(out);
				}
				line_len = 0;
			} else if (c == '\r') {
				/* ignore carriage returns */
			} else if (line_len < sizeof(line) - 1) {
				line[line_len++] = c;
			}
		}
	}
	return NULL;
}

/* input thread: read keyboard lines and send them to the server. */
static void *input_thread(void *arg)
{
	(void)arg;
	char in[BUF_SIZE];

	while (running && fgets(in, sizeof(in), stdin)) {
		size_t len = strlen(in);
		while (len > 0 && (in[len - 1] == '\n' || in[len - 1] == '\r'))
			in[--len] = '\0';
		if (len == 0)
			continue;

		if (strcmp(in, "/quit") == 0) {
			char msg[16];
			snprintf(msg, sizeof(msg), "QUIT\n");
			if (send(sock, msg, strlen(msg), 0) < 0)
				running = 0;
			break;
		} else if (strcmp(in, "/list") == 0) {
			if (send(sock, "LIST\n", 5, 0) < 0)
				running = 0;
		} else {
			/* keep the text within MAX_MSG so the server accepts it */
			if (len > (size_t)(MAX_MSG - 8))
				len = MAX_MSG - 8;
			in[len] = '\0';
			char msg[MAX_NAME + BUF_SIZE + 16];
			snprintf(msg, sizeof(msg), "MSG %s\n", in);
			if (send(sock, msg, strlen(msg), 0) < 0)
				running = 0;
		}
	}
	running = 0;
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 4) {
		fprintf(stderr, "usage: %s <host> <port> <username>\n", argv[0]);
		return 1;
	}

	const char *host  = argv[1];
	int         port  = atoi(argv[2]);
	const char *user  = argv[3];

	if (port <= 0 || port > 65535) {
		fprintf(stderr, "invalid port: %s\n", argv[2]);
		return 1;
	}
	if ((int)strlen(user) >= MAX_NAME) {
		fprintf(stderr, "username too long (max %d chars)\n", MAX_NAME - 1);
		return 1;
	}

	printf("[client %s] my pid=%d, my parent (launcher) pid=%d\n", user, (int)getpid(), (int)getppid());
	fflush(stdout);

	/* ignore SIGPIPE so a write to a closed socket won't terminate us */
	signal(SIGPIPE, SIG_IGN);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons((uint16_t)port);
	if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
		fprintf(stderr, "invalid host: %s\n", host);
		return 1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(sock);
		return 1;
	}

	/* protocol: JOIN is the first line the client sends */
	char join[MAX_NAME + 8];
	snprintf(join, sizeof(join), "JOIN %s\n", user);
	if (send(sock, join, strlen(join), 0) < 0) {
		perror("send JOIN");
		close(sock);
		return 1;
	}

	pthread_t rt, it;
	if (pthread_create(&rt, NULL, recv_thread, NULL) != 0) {
		perror("pthread_create recv");
		close(sock);
		return 1;
	}
	if (pthread_create(&it, NULL, input_thread, NULL) != 0) {
		perror("pthread_create input");
		running = 0;
		shutdown(sock, SHUT_RDWR);
		pthread_join(rt, NULL);
		close(sock);
		return 1;
	}

	/* wait for the user to type /quit (or EOF on stdin) */
	pthread_join(it, NULL);
	running = 0;
	shutdown(sock, SHUT_RDWR); /* wake up recv_thread if it is blocked */
	pthread_join(rt, NULL);

	close(sock);
	safe_print("[client] disconnected\n");
	return 0;
}
