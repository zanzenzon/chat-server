#ifndef LOGGER_H
#define LOGGER_H

int  logger_init(const char *dir);
void log_message(const char *sender, const char *text);
void log_event(const char *fmt, ...);
void logger_close(void);

#endif
