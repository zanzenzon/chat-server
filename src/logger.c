#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "logger.h"

static FILE *chat_fp = NULL;
static FILE *event_fp = NULL;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned long msg_count = 0;
static unsigned long event_count = 0;

/* Writes the current time as "YYYY-MM-DD HH:MM:SS" into buf. */
static void make_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

int logger_init(const char *dir) {
    char path[512];

    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        perror("logger_init: mkdir");
        return -1;
    }

    snprintf(path, sizeof path, "%s/chat_history.txt", dir);
    chat_fp = fopen(path, "a");

    snprintf(path, sizeof path, "%s/events.log", dir);
    event_fp = fopen(path, "a");

    if (!chat_fp || !event_fp) {
        perror("logger_init: fopen");
        return -1;
    }
    return 0;
}

void log_message(const char *sender, const char *text) {
    char ts[32];
    make_timestamp(ts, sizeof ts);

    pthread_mutex_lock(&log_lock);
    fprintf(chat_fp, "[%s] %s: %s\n", ts, sender, text);
    fflush(chat_fp);
    msg_count++;
    pthread_mutex_unlock(&log_lock);
}

void log_event(const char *fmt, ...) {
    char ts[32];
    char msg[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    make_timestamp(ts, sizeof ts);

    pthread_mutex_lock(&log_lock);
    fprintf(event_fp, "[%s] %s\n", ts, msg);
    fflush(event_fp);
    event_count++;
    pthread_mutex_unlock(&log_lock);
}

void logger_close(void) {
    pthread_mutex_lock(&log_lock);
    if (chat_fp)  { fclose(chat_fp);  chat_fp = NULL; }
    if (event_fp) { fclose(event_fp); event_fp = NULL; }
    pthread_mutex_unlock(&log_lock);
}