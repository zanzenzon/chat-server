#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "common.h"


static void *client_thread(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    printf("[server] client fd=%d connected\n", fd);

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof buf, 0)) > 0) {
        send(fd, buf, (size_t)n, 0); /* still just an echo for now*/
    }

    printf("[server] client fd=%d disconnected\n", fd);
    close(fd);
    return NULL;
}

int main(void) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(listen_fd, 10) < 0) { perror("listen"); return 1; }
    printf("[server] listening on port %d\n", PORT);

    /* the accept loop */
    for (;;) {
        int fd = accept(listen_fd, NULL, NULL);
        if (fd < 0) { perror("accept"); continue; }

        int *arg = malloc(sizeof *arg);   /* private copy of fd for the new thread */
        if (!arg) { close(fd); continue; }
        *arg = fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
            perror("pthread_create");
            close(fd);
            free(arg);
            continue;
        }
        pthread_detach(tid);
    }
}
