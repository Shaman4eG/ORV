#include <stdarg.h>
#include "header.h"

void eventLog(FILE * stream, const char * fmt, ...) {
  va_list args, args2;
  va_start(args, fmt);
  va_copy(args2, args);
  vfprintf(stream, fmt, args);
  vfprintf(stdout, fmt, args2);
  va_end(args);
}

ssize_t continiousWrite (int fd, const void *buf, size_t len) {
  ssize_t ret, sndd = 0;
  unsigned char *addr = (unsigned char *)buf;
  //ret = write(fd, buf, len);
  while(len != 0 && (ret = write(fd, addr, len)) != 0) {
    if(ret == -1) {
      if(errno == EINTR)
        continue;
      return -1;
    }

    len -= ret;
    addr += ret;
    sndd += ret;
  }
  return sndd;
}

int send(void * self, local_id dst, const Message * msg) {
  pipeDescriptor *pd = ( pipeDescriptor *)self;
  size_t len = sizeof (MessageHeader) + msg->s_header.s_payload_len * sizeof(char);

  if(continiousWrite (pd->out, msg, len) == -1) {
      perror("send error\n");
      return -1;
  }

  return 0;
}

int send_multicast(void * self, const Message * msg) {
  Process *pd = ( Process *)self;
  for( int i = 0; i < pd->count; i++) {
    if(pd->pipes[i].out != -1) {
      if(send(&pd->pipes[i], i, msg) == -1) {
        perror("multicast error\n");
        return -1;
      }
    }
  }

  return 0;
}

ssize_t continiousRead (int fd, void *buf, size_t len) {
  ssize_t ret;
  size_t rcvd = 0;
  unsigned char * addr = (unsigned char *)buf;
  //ret = read(fd , buff, len);
  while (len != 0 && (ret = read(fd , addr, len)) != 0) {
    if (ret == -1 ) {
      if (errno == EINTR)
        continue;
      else if (errno == EAGAIN)
        return -2;
      else
        return -1;
    }

    len  -= ret;
    addr += ret;
    rcvd += ret;
  }
  return rcvd;
}

int receive(void * self, local_id from, Message * msg) {

  pipeDescriptor *pd = (pipeDescriptor *)self;
  MessageHeader msgHeader;
  size_t len = sizeof (MessageHeader);
  ssize_t ret;

  ret = continiousRead(pd->in, &msgHeader, len);
  if(ret == -1) {
      perror ("receive error\n");
      return ret;
  }
  else if (ret == -2)
    return ret;

  len = msgHeader.s_payload_len;

  if(len > 0) {
    char *payload = malloc (len * sizeof msgHeader.s_payload_len);

    ret = continiousRead(pd->in, payload, len);
    if ( ret == -1) {
      perror("receive payload error");
      return ret;
    }
    else if (ret == -2)
      return ret;

    strcpy(msg->s_payload, payload);
    free (payload);
  }
  else {
    strcpy(msg->s_payload, "");
  }
  msg->s_header = msgHeader;

  return 0;
}
int receive_any(void * self, Message * msg) {

  Process *pd = ( Process *)self;
  ssize_t ret = -1;
  for(int i = 1; i < pd->count; i++) {
    if(pd->pipes[i].in != -1) {
      if((ret = receive(&pd->pipes[i], i, msg)) == -1) {
        perror("any errpr\n");
        return -1;
      }
      else if (ret == 0) break;
      else if (ret == -2) {
      }
    }
  }

  if (ret != 0) return -2;

  return 0;
}
int receiveAll(Process *cp, int count, MessageType type, int *anotherCnt) {

  Message msg;
  int rcv = 0;
  int arcv = 0;

  //while (rcv != count) {
  for(int i = 1; i < cp->count && rcv < count; i++)
    if(cp->pipes[i].in != -1) {
      if (receive(&cp->pipes[i], i, &msg) == 0) {
        if (msg.s_header.s_magic == MESSAGE_MAGIC) {
          if(msg.s_header.s_type == type)
              rcv++;
          else {
              arcv++;
              fprintf(stderr, "unhandled message type %d\n", msg.s_header.s_type);
            }
          }
      }
      else {
        perror("parent receive_any error");
      }

    }

  *anotherCnt = arcv;
  return 0;
}
