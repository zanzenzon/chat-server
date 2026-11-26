#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"

/* ------------------------------------------------------------------ launcher
 *
 * Phase 1 process manager.  Starts the server and N client processes as
 * separate children (fork + execv), tracks every PID, then reaps them with
 * waitpid() reporting each exit status.  No zombies are left behind.
 *
 * Usage:  ./launcher [num_clients]
 *
 * The parent:
 *   1. forks once for the server (execv ./bin/server)
 *   2. sleeps briefly so the server is ready
 *   3. forks once per client (execv ./bin/client 127.0.0.1 <port> clientN)
 *   4. prints a table of PIDs
 *   5. loops on waitpid() until every child is reaped, printing how each
 *      child exited (WIFEXITED / WEXITSTATUS / WIFSIGNALED / WTERMSIG)
 *
 * Ctrl+C (SIGINT) asks the launcher to terminate every child and reap them.
 */

#define MAX_PROCS (MAX_CLIENTS + 1) /* server + all clients */

static pid_t pids[MAX_PROCS];
static int   nprocs = 0;
static const char *roles[MAX_PROCS];
static volatile int stop = 0;

static void handle_sigint(int sig)
{
	(void)sig;
	stop = 1;
	for (int i = 0; i < nprocs; i++)
		if (pids[i] > 0)
			kill(pids[i], SIGTERM);
}

int main(int argc, char **argv)
{
	int num_clients = 3;
	if (argc > 1) {
		num_clients = atoi(argv[1]);
		if (num_clients < 1)  num_clients = 1;
		if (num_clients > MAX_CLIENTS) num_clients = MAX_CLIENTS;
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigint;
	sigaction(SIGINT, &sa, NULL);

	/* -------- 1. fork + exec the server -------- */
	pid_t spid = fork();
	if (spid == 0) {
		/* child */
		char *args[] = { "./bin/server", NULL };
		execv(args[0], args);
		perror("execv ./bin/server");
		_exit(1);
	}
	if (spid < 0) {
		perror("fork server");
		return 1;
	}
	pids[nprocs] = spid;
	roles[nprocs] = "server";
	nprocs++;
	printf("[launcher] server    pid=%d\n", (int)spid);

	/* short sleep so the server is ready to accept connections */
	sleep(1);

	/* -------- 2. fork + exec one client per requested slot -------- */
	for (int i = 0; i < num_clients; i++) {
		pid_t cp = fork();
		if (cp == 0) {
			/* child */
			char port[16];
			char name[32];
			snprintf(port, sizeof(port), "%d", PORT);
			snprintf(name, sizeof(name), "client%d", i + 1);
			char *args[] = { "./bin/client", "127.0.0.1", port, name, NULL };
			execv(args[0], args);
			perror("execv ./bin/client");
			_exit(1);
		}
		if (cp < 0) {
			perror("fork client");
			return 1;
		}
		pids[nprocs] = cp;
		roles[nprocs] = "client";
		nprocs++;
		printf("[launcher] client%d  pid=%d\n", i + 1, (int)cp);
		usleep(200000); /* small stagger so the server can accept in order */
	}

	/* -------- 3. print the PID table -------- */
	printf("\n[launcher] process table (%d children):\n", nprocs);
	printf("%-8s %-10s %s\n", "PID", "ROLE", "STATUS");
	for (int i = 0; i < nprocs; i++)
		printf("%-8d %-10s running\n", (int)pids[i], roles[i]);
	fflush(stdout);

	/* -------- 4. waitpid: reap every child, report exit status -------- */
	int remaining = nprocs;
	while (remaining > 0 && !stop) {
		int status;
		pid_t w = waitpid(-1, &status, 0);
		if (w < 0) {
			if (errno == EINTR)
				continue;
			if (errno == ECHILD)
				break;
			perror("waitpid");
			break;
		}
		remaining--;
		const char *role = "unknown";
		for (int i = 0; i < nprocs; i++) {
			if (pids[i] == w) { role = roles[i]; break; }
		}
		if (WIFEXITED(status))
			printf("[launcher] pid=%-6d (%s) exited with status %d\n",
			       (int)w, role, WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			printf("[launcher] pid=%-6d (%s) killed by signal %d\n",
			       (int)w, role, WTERMSIG(status));
		else
			printf("[launcher] pid=%-6d (%s) status 0x%x\n",
			       (int)w, role, status);
		fflush(stdout);
	}

	/* -------- 5. on Ctrl+C, terminate and reap anything left -------- */
	if (stop) {
		for (int i = 0; i < nprocs; i++)
			if (pids[i] > 0)
				kill(pids[i], SIGTERM);
		while (remaining > 0) {
			int status;
			pid_t w = waitpid(-1, &status, 0);
			if (w < 0) { if (errno == ECHILD) break; continue; }
			remaining--;
		}
	}

	printf("[launcher] all children reaped. bye.\n");
	return 0;
}
