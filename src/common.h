#ifndef COMMON_H
#define COMMON_H

#define PORT 5000
#define MAX_CLIENTS 20
#define MAX_NAME 32
#define MAX_MSG 512
#define BUF_SIZE 1024

/* protocol: one text line per message, ending in \n 
	client -> server: JOIN <name> | MSG <text> | LIST | QUIT
	server -> client: plain text lines (chat, notices, user list) */

#endif
