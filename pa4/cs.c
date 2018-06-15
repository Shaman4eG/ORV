#include "header.h"
#include "queue.h"

int request_cs (const void *self) {

  ProcessInfo *pi = (ProcessInfo *)self;
  noteEventForLamportTime();
  Message *msg = createMessage (NULL, 0, CS_REQUEST, get_lamport_time());
  if(send_multicast (pi, msg) == 1){
    fprintf(stderr, "[id: %d] request_cs send_multicast error\n", pi->id);
    return -1;
  }
  //printf("%d after multicast\n", pi->id);
  //fflush(stdout);

  initQueue (pi->process->count);
  //printf("after initQueue\n");
  //fflush(stdout);

  Node *node = malloc (sizeof (Node));
  node->id = pi->id;
  node->time = get_lamport_time();

  //printf("before enqueue %d\n", node->id);
  //fflush(stdout);
  enqueue (node);
  //printf("after enqueue %d\n", node->id);
  //fflush(stdout);

  int replied = 0;
  local_id sender;
  Node *minNode;

  while (replied < (pi->workers - 1) || ((minNode = queueFont()) && minNode->id != pi->id)) {
    if ((sender = receive_any (pi, msg)) == -1) {
      fprintf(stderr, "[id: %d] request_cs receive_any  error\n", pi->id);
      return -1;
    }
    synchronizeLamportTime (msg->s_header.s_local_time);
    switch (msg->s_header.s_type) {
      case CS_REQUEST:
        //printf("after receive request\n");
        //fflush(stdout);
        node = malloc (sizeof (Node));
        node->id = sender;
        node->time = msg->s_header.s_local_time;
        enqueue(node);
        noteEventForLamportTime();
        msg->s_header.s_type = CS_REPLY;
        msg->s_header.s_local_time = get_lamport_time();
        if (send(pi, sender, msg) == -1) {
          fprintf(stderr, "[id: %d] request_cs send error\n", pi->id);
          return -1;
        }
        //printf("after send\n");
        //fflush(stdout);
        break;
      case CS_REPLY:
        replied++;
        //printf("%d reply received %d\n", pi->id, replied);
        //fflush(stdout);
        //if (replied == pi->workers - 1)
        //  printf("%d font is %d\n", pi->id, queueFont()->id);
        //if (!(replied < (pi->process->count - 1) || ((minNode = queueFont()) && minNode->id != pi->id))) {
        //  printf("fuck %d\n", pi->process->count);
        //  fflush(stdout);
        //}
        break;
      case CS_RELEASE:
        dequeue();
        break;
      case DONE:
        pi->workers--;
      default:
        break;
    }
  }
  //printf("here\n");
  //fflush(stdout);

  return 0;
}

int release_cs (const void *self) {
  ProcessInfo *pi = (ProcessInfo *)self;

  dequeue();

  noteEventForLamportTime();
  Message *msg = createMessage( NULL, 0, CS_RELEASE, get_lamport_time());
  if (send_multicast(pi, msg) == -1) {
        fprintf(stderr, "[id: %d] request_cs send_multicast error\n", pi->id);
        return -1;
  }
  return 0;
}
