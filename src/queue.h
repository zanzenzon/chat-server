#ifndef QUEUE_H
#define QUEUE_H
#include "common.h"

typedef struct message {
    char sender[MAX_NAME];
    char *text;
    long timestamp;
    struct message *next;
} message_t;

void       msgq_init(void);
void       msgq_push(const char *sender, const char *text);
message_t *msgq_pop(void);
void       msgq_destroy(void);

#endif
