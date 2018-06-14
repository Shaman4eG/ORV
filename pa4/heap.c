#include <stdbool.h>

#include "heap.h"
#include <stdlib.h>
#include <stdio.h>

static Node **_nodes;
static int _heapSize;
static int _heapCurrentSize;
static less_cb _less;

void siftUp (int v);
void siftDown (int v);
void exchange(int i, int j);
int parent(int v);

void initHeap (int nodesCount, less_cb less)
  {
      _heapSize = nodesCount;
      _nodes = malloc(_heapSize * sizeof (Node *));
      _heapCurrentSize = 0;
      _less = less;
  }

void insertNode(Node *node)
{
    if (_heapCurrentSize >= _heapSize) {
      printf("fuck you\n");
      fflush(stdout);
      _heapSize = _heapSize * 2;
      _nodes = realloc(_nodes, _heapSize);
    }
    _nodes[_heapCurrentSize] = node;
    _heapCurrentSize++;
    siftUp(_heapCurrentSize - 1);
}

Node * lookAtMin() {
    if (_heapCurrentSize == 0) {
      return NULL;
    }
    return _nodes[0];
}

Node * extractMin()
{
    if (_heapCurrentSize == 0) {
      return NULL;
    }
    Node *minNode = _nodes[0];
    _nodes[0] = _nodes[_heapCurrentSize - 1];
    _heapCurrentSize--;
    if (_heapCurrentSize > 0)
        siftDown(0);

    return minNode;
}
 void siftDown(int v) {
    int i;
    while (v * 2 + 1 < _heapCurrentSize) {
        i = v * 2 + 1;
        if (i + 1 < _heapCurrentSize) {
            if(_less(_nodes[i + 1], _nodes[i])) i++;
        }
        if(!_less(_nodes[i], _nodes[v])) break;
        exchange(v,i);
        v = i;
    }
}

 void siftUp(int v){
    int prnt = parent(v);
    while (v > 0 && _less(_nodes[v], _nodes[prnt])) {
        exchange(prnt, v);
        v = prnt;
        prnt = parent(v);
    }
}

 int parent(int v){
    int parent = v / 2;
    if(v % 2 == 0 && parent != 0) parent--;
    return parent;
}

 void exchange(int i, int j){
    Node *buff = _nodes[i];
    _nodes[i] = _nodes[j];
    _nodes[j] = buff;
}


