#include <sys/stat.h>
#include <errno.h>
#include "header.h"

Message *createMessage(const char* playload, int playloadLength, MessageType type) {

  Message *msg = malloc(sizeof (Message));

  msg->s_header.s_magic = MESSAGE_MAGIC;
  msg->s_header.s_payload_len = playloadLength;
  msg->s_header.s_type = type;
  msg->s_header.s_local_time = (timestamp_t) time(NULL);

  strcpy(msg->s_payload, playload);

  return msg;
}

int child(int id, Process *pipesList, FILE *flog, FILE *ef) {

  if (ef == NULL) {
    perror("events.log file open error");
    return -1;
  }
  pid_t pid = getpid();
  pid_t ppid = getppid();
  int rcvDone = 0;


  char messageStarted[49];
  int length = sprintf(messageStarted, log_started_fmt, id, pid, ppid );
  if (length < 0) {
    perror("sprintf error");
    return -1;
  }
  const Message *msg = createMessage(messageStarted, length, STARTED);
  //printf("%zu\n", pipesList->count);
  //printf("%d\n", pipesList->pipes[0].out);

  if(send_multicast(pipesList, msg) == -1) {
   perror("send_multicast error");
   return -1;
  }
  //printf("here7\n");
  eventLog(ef, log_started_fmt, id, pid, ppid);
  free((char *)msg);

  if (receiveAll(pipesList, pipesList->count -2, STARTED, &rcvDone) == -1){
    perror("receiveAll Started child");
    return -1;
  }
  //if(rcvCount != pipesList->count - 2) {
  // perror("received count is wrog");
  // return -1;
  //}
  eventLog(ef, log_received_all_started_fmt, id);
  char messageDone[29];
  length = sprintf(messageDone, log_done_fmt, id);
  if(length < 0) {
    perror("sprintf done error");
    return -1;
  }
  msg = createMessage(messageDone, length, DONE);

  if(send_multicast(pipesList, msg) == -1) {
    perror("send done error");
    return -1;
  }
  eventLog(ef, log_done_fmt, id);

  if(receiveAll(pipesList, pipesList->count - 2 - rcvDone, DONE,  &rcvDone) == -1) {
    perror("receiveAll Done child");
    return -1;
  }

  //if(rcvCount != pipesList->count - 2)
  //  error("received count is wrog");
  eventLog(ef, log_received_all_done_fmt, id);
  closeUsedPipes(id, pipesList, flog);

  free((char *)msg);
  fclose(ef);
  fclose(flog);
  return 0;
}
