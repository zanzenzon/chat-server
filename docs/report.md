# Multi-Client Chat Server: Logging and Storage
## 1. Logger architecture (src/logger.c)

The logger records every chat message and every join, leave, and error event to disk.
It exposes four functions declared in src/logger.h:

- `logger_init(dir)` (line 26) creates the log folder if needed and opens
  `chat_history.txt` and `events.log` in append mode.
- `log_message(sender, text)` (line 47) writes one chat line.
- `log_event(fmt, ...)` (line 58) writes one event line, formatted like printf.
- `logger_close()` (line 76) flushes and closes both files.

A private helper, `make_timestamp()` (line 19), formats the current time as
`YYYY-MM-DD HH:MM:SS` for every line. The server's client threads and broadcaster
call these functions; the logger itself does not touch any sockets.

## 2. File storage (OS concept)

Chat history and events are stored on disk so they survive a server restart.
The files are opened once in `logger_init` (lines 34-38) with mode "a" (append),
so new lines are added to the end and earlier records are kept.

## 3. Mutex protection (OS concept)

Many client threads can log at the same moment. Without protection, two writes
could interleave and produce half-lines. The shared state is protected by
`log_lock` (declared line 14). Each write is wrapped in a lock and unlock:

- `log_message`: lines 51 and 55
- `log_event`: lines 69 and 73
- `logger_close`: lines 77 and 80

The timestamp is formatted before the lock is taken (lines 48-49 and 67), because it
does not touch shared data, which keeps the critical section short.

## 4. Buffered writes and flushing (OS concept)

C buffers output in memory. Each write calls `fflush` (lines 53, 71) to push the
line to the operating system immediately. This means a crash loses at most the line
being written, not the buffered lines before it.

## 5. Counters (for /stats)

`msg_count` and `event_count` (lines 15-16) count every logged message and event.
These feed the `/stats` command, which will read them through a getter to be
agreed with the team.
