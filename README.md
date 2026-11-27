# chat-server
Multi-Client Chat Server (Phase 1: Process Management)

## Description
A chat system made of a launcher, a server and several clients. In Phase 1 the launcher acts a a process manager: it creates the server and the clients as
seperate processes, identifies them, runs the right program in each, and waits for them to finish.

Note: in Phase 1 the server is still an echo server. The chat protocol (JOIN/MSG/LIST/QUIT), the message queue and the logger are not connected to it yet.
They are built and tested seperately and will be integrated in a later phase.

## Role of each process
launcher : Parent. Creates the server and clients, prints the PID table, waits for them with waitpid, handles Ctrl+C.
server : Child of the launcher. Listen on port 5000, handles each clients in a thread.
client : Child of the launcher. One chat user: reads the keyboard and the prients incoming messages.

## System calls used
fork() | launcher | create one child for the server and one per client.
execv() | launcher (in each child) | replace the child with ./bin/server or ./bin/client.
getpid() | launcher, server, client | each process prints its own PID.
getppid() | launcher, server, client | shows the parent-child relationship (the parent of server and clients in the launcher)
waitpid() | launcher |  wait for each child, collect exit status,  avoid zombies.
WIFEXITED / WEXITSTATUS / WIFSIGNALED / WTERMSIG | launcher | report how each child ended.
sigaction() + kill() | launcher | on Ctrl+C, send SIGTERM to all children, then reap them.
_exit() | launcher (child) | leave immediately if execv fails, without running the parent's cleanup code.

## Build
,,,
make
,,,

## Run
,,,
./bin/launcher 1
,,,
the number is how many clients to start (default 3). because all clients started by the launcher share one keyboard,

use './bin/launcher 1' and start more clients by hands in other terminals:
,,,
./bin/client 127.0.0.1 alice
,,,
the launcher keeps running while the server is alive (the server runs forever).
this is normal, not a hang.

## Stop
press Ctrl+C int the launcher terminal. it sends SIGTERM to all children, reaps them, and prints their exit status. 
type /quit in a client to end only that client.

## Showing the process tree
in another terminal while it runs:
,,,
pastree -p $(pgrep launcher)
ps -ef --forest | grep -E "launcher|server|client"
,,,
check for zombies after stopping:
,,,
ps aux | grep defunct
,,,
