
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static message_t *q_head = NULL;
static message_t *q_tail = NULL;

static char *dup_text(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        perror("malloc");
        return NULL;
    }
    memcpy(copy, src, len + 1);
    return copy;
}

void msgq_init(void) {
    q_head = NULL;
    q_tail = NULL;
}

void msgq_push(const char *sender, const char *text) {
    if (sender == NULL || text == NULL) {
        return;
    }

    message_t *node = malloc(sizeof(*node));
    if (node == NULL) {
        perror("malloc");
        return;
    }

    snprintf(node->sender, MAX_NAME, "%s", sender);
    node->text = dup_text(text);
    node->timestamp = (long)time(NULL);
    node->next = NULL;

    if (node->text == NULL) {
        free(node);
        return;
    }

    if (q_tail == NULL) {
        q_head = node;
        q_tail = node;
    } else {
        q_tail->next = node;
        q_tail = node;
    }
}

message_t *msgq_pop(void) {
    if (q_head == NULL) {
        return NULL;
    }

    message_t *node = q_head;
    q_head = node->next;
    if (q_head == NULL) {
        q_tail = NULL;
    }
    node->next = NULL;
    return node;
}

void msgq_destroy(void) {
    message_t *curr = q_head;
    while (curr != NULL) {
        message_t *next = curr->next;
        free(curr->text);
        free(curr);
        curr = next;
    }
    q_head = NULL;
    q_tail = NULL;
}
