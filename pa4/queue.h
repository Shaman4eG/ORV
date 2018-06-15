#ifndef __DISTRIBUTED_CLASS_QUEUE__H
#define __DISTRIBUTED_CLASS_QUEUE__H

#include "ipc.h"

typedef struct {
  local_id id;
  timestamp_t time;
} Node;

void initQueue(int count);
void enqueue (Node *node);
Node * queueFont();
Node * dequeue();

#endif
