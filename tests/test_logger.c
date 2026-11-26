#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "logger.h"

#define THREADS     4
#define PER_THREAD  1000
#define TEST_DIR    "test_logs"

/* Each thread logs many messages and events at the same time. */
static void *worker(void *arg) {
    int id = *(int *)arg;
    char name[32];
    snprintf(name, sizeof name, "user%d", id);

    for (int i = 0; i < PER_THREAD; i++) {
        log_message(name, "hello from the test");
        log_event("user %s did event %d", name, i);
    }
    return NULL;
}

/* Counts the lines in a file and checks that each one starts with '['. */
static int check_file(const char *path, int expected, const char *label) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("FAIL %s: cannot open %s\n", label, path);
        return 0;
    }

    char line[2048];
    int count = 0;
    int bad = 0;
    while (fgets(line, sizeof line, fp)) {
        count++;
        if (line[0] != '[') bad++;   /* a garbled line would not start with a timestamp */
    }
    fclose(fp);

    int ok = (count == expected && bad == 0);
    printf("%s %s: %d lines (expected %d), %d malformed\n",
           ok ? "PASS" : "FAIL", label, count, expected, bad);
    return ok;
}

int main(void) {
    /* Logs are appended, so delete old test files first. */
    remove(TEST_DIR "/chat_history.txt");
    remove(TEST_DIR "/events.log");

    if (logger_init(TEST_DIR) != 0) {
        printf("FAIL: logger_init\n");
        return 1;
    }

    pthread_t tids[THREADS];
    int ids[THREADS];
    for (int i = 0; i < THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&tids[i], NULL, worker, &ids[i]) != 0) {
            printf("FAIL: pthread_create\n");
            return 1;
        }
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(tids[i], NULL);
    }

    logger_close();

    int expected = THREADS * PER_THREAD;
    int ok1 = check_file(TEST_DIR "/chat_history.txt", expected, "chat_history");
    int ok2 = check_file(TEST_DIR "/events.log", expected, "events.log");

    return (ok1 && ok2) ? 0 : 1;
}
