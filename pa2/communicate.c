#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include "header.h"
#include <stdarg.h>

void writeLog(FILE *stream, const char *fmt, ...) {
  va_list args, args2;
  va_start(args, fmt);
  va_copy(args2, args);
  vfprintf(stream, fmt, args);
  vfprintf(stdout, fmt, args2);
  va_end(args);
}

Message *createMessage(const char *playload, int playloadLength,
                       MessageType type, timestamp_t time) {

  Message *msg = malloc(sizeof(Message));

  msg->s_header.s_magic = MESSAGE_MAGIC;
  msg->s_header.s_payload_len = playloadLength;
  msg->s_header.s_type = type;
  msg->s_header.s_local_time = time;

  if (playload != NULL)
    memcpy(msg->s_payload, playload, playloadLength);

  return msg;
}

ssize_t continiousWrite(int fd, const void *buf, size_t len) {
  ssize_t ret;
   ssize_t sndd = 0;
   unsigned char *addr = (unsigned char *)buf;
  //ret = write(fd, buf, len);
   while (len != 0 && (ret = write (fd, addr, len)) != 0) {
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      return -1;
    }

    len -= ret;
    addr += ret;
    sndd += ret;
  }
   return sndd;
  //return ret;
}

int send(void *self, local_id dst, const Message *msg) {
  Process *pi = (Process *)self;
  size_t len =
      sizeof(MessageHeader) + msg->s_header.s_payload_len * sizeof(char);

  if (continiousWrite(pi->arrayOfPipes->pipes[dst].out, msg, len) == -1) {
    perror("send error\n");
    return -1;
  }
  return 0;
}

int send_multicast(void *self, const Message *msg) {
  Process *pi = (Process *)self;
  for (local_id i = 0; i < pi->arrayOfPipes->count; i++) {
    if (pi->arrayOfPipes->pipes[i].out != -1) {
      if (send(self, i, msg) == -1) {
        perror("multicast error\n");
        return -1;
      }
    }
  }

  return 0;
}

ssize_t continiousRead(int fd, void *buf, size_t len) {
  ssize_t ret;
  ssize_t rcvd = 0;
  unsigned char *addr = (unsigned char *)buf;
  // ret = read(fd, buf, len);
  while (len != 0 && (ret = read(fd, addr, len)) != 0) {
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      // return -1;
      else
        return -1;
    }

    len -= ret;
    addr += ret;
    rcvd += ret;
  }
  return rcvd;
  // return ret;
}

int receiveOne(void *self, local_id from, Message *msg) {

  Process *pi = (Process *)self;
  MessageHeader msgHeader;
  size_t len = sizeof(MessageHeader);
  ssize_t ret;

  ret = continiousRead(pi->arrayOfPipes->pipes[from].in, &msgHeader, len);
  if (ret <= 0) {
    return -1;
  }

  len = msgHeader.s_payload_len;

  if (len > 0) {
    char payload[len];

    ret = read(pi->arrayOfPipes->pipes[from].in, &payload, len);
    if (ret == -1) {
      perror("receive payload error");
      return -2;
    }

    memcpy(msg->s_payload, payload, len);
  }

  msg->s_header = msgHeader;

  return 0;
}

int receive(void *self, local_id from, Message *msg) {
  int ret;

  while (3 < 5) {
    ret = receiveOne(self, from, msg);
    if (ret == -1) {
      usleep((unsigned int)100);
      continue;
    } else if (ret == -2)
      return -1;
    else if (ret == 0)
      return 0;
  }
}
int receive_any(void *self, Message *msg) {

  Process *pi = (Process *)self;
  ssize_t ret;
  while (3 < 5) {
    for (local_id i = 0; i < pi->arrayOfPipes->count; i++) {
      if (pi->arrayOfPipes->pipes[i].in != -1) {
        if ((ret = receiveOne(self, i, msg)) == -1) {
          usleep((unsigned int)100);
          continue;
        } else if (ret == -2) {
          return -1;
        } else if (ret == 0)
          return 0;
      }
    }
    usleep((unsigned int)100);
  }
}

int receiveAll(Process *pi, int count, MessageType type, void *buf,
               handlerForBuffer bHandler) {

  Message msg;
  int rcv = 0;
  ssize_t ret;

  for (local_id i = 1; i <= count; i++) {
    if (pi->arrayOfPipes->pipes[i].in != -1) {
      ret = receive(pi, i, &msg);
      if (ret == 0) {
        if (msg.s_header.s_magic == MESSAGE_MAGIC) {
          if (msg.s_header.s_type == type) {
            if (buf != NULL && bHandler != NULL) {
              bHandler(buf, &msg, rcv);
            }
            rcv++;
          } else {
            fprintf(stderr, "unhandled message type %d instead of %d\n",
                    msg.s_header.s_type, type);
            return -1;
          }
        } else {
          fprintf(stderr, "corrupted magic value %d instead of %d\n",
                  msg.s_header.s_magic, MESSAGE_MAGIC);
          return -1;
        }
      } else if (ret == -1) {
        perror("parent receiveAll error");
        return -1;
      }
    }
  }
  return 0;
}
