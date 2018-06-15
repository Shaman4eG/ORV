#ifndef __DISTRIBUTED_CLASS_HEAP__H
#define __DISTRIBUTED_CLASS_HEAP__H

#include "queue.h"
#include <stdbool.h>

typedef bool (*less_cb) (void *v, void *b);

void insertNode (Node *node);
Node *lookAtMin();
Node *extractMin();
void initHeap (int nodeCount, less_cb less);

#endif
