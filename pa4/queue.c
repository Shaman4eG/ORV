#include <stdbool.h>

#include "queue.h"
#include "heap.h"
#include <stdio.h>

bool NodeLess (void *v, void *i) 
{
	Node *fstNode = (Node *)v;
	Node *scndNode = (Node *)i;

	if (fstNode->time < scndNode->time
		|| (fstNode->time == scndNode->time && fstNode->id < scndNode->id))
	return true;
	else
	return false;
}

void initQueue (int count) 
{
	initHeap (count, NodeLess);
}

void enqueue (Node *node) 
{
	insertNode (node);
}

Node * queueFont() 
{
	Node * node = lookAtMin();
	return node;
}

Node * dequeue() 
{
	return extractMin();
}


