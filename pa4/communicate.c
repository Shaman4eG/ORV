#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include "header.h"
#include <stdarg.h>

void writeLog(FILE *stream, const char *fmt, ...)
{
	  va_list args, args2;
	  va_start(args, fmt);
	  va_copy(args2, args);
	  vfprintf(stream, fmt, args);
	  vfprintf(stdout, fmt, args2);
	  va_end(args);
}

Message *createMessage(const char *playload, int playloadLength, MessageType type, timestamp_t time) 
{
	  Message *msg = malloc(sizeof(Message));
	  msg->s_header.s_magic = MESSAGE_MAGIC;
	  msg->s_header.s_payload_len = playloadLength;
	  msg->s_header.s_type = type;
	  msg->s_header.s_local_time = time;
	  if (playload != NULL)
		memcpy(msg->s_payload, playload, playloadLength);
	  return msg;
}

ssize_t continiousWrite(int fd, const void *buf, size_t len) 
{
	ssize_t returned;
	ssize_t bytesSended = 0;
	unsigned char *addr = (unsigned char *)buf;
	while (len != 0 && (returned = write (fd, addr, len)) != 0) 
	{
		if (returned == -1) 
		{
			if (errno == EINTR)
			continue;
			return -1;
		}

		len -= returned;
		addr += returned;
		bytesSended += returned;
	}
	return bytesSended;
}

int send(void *self, local_id dst, const Message *msg) 
{
	ProcessInfo *pi = (ProcessInfo *)self;
	size_t len = sizeof(MessageHeader) + msg->s_header.s_payload_len * sizeof(char);

	if (continiousWrite(pi->process->pipes[dst].out, msg, len) == -1) 
	{
		perror("send error\n");
		return -1;
	}
	return 0;
}

int send_multicast(void *self, const Message *msg) 
{
	ProcessInfo *pi = (ProcessInfo *)self;
	for (local_id i = 0; i < pi->process->count; i++) 
	{
		if (pi->process->pipes[i].out != -1) 
		{
			if (send(self, i, msg) == -1) 
			{
				perror("multicast error\n");
				return -1;
			}
		}
	}

	return 0;
}

ssize_t continiousRead(int fd, void *buf, size_t len) 
{
	ssize_t returned;
	ssize_t recieved = 0;
	unsigned char *addr = (unsigned char *)buf;
	while (len != 0 && (returned = read(fd, addr, len)) != 0) 
	{
		if (returned == -1) 
		{
			if (errno == EINTR)
			continue;
			else
			return -1;
		}

		len -= returned;
		addr += returned;
		recieved += returned;
	}
	return recieved;
}

int receiveOne(void *self, local_id from, Message *msg) 
{
	ProcessInfo *pi = (ProcessInfo *)self;
	MessageHeader msgHeader;
	size_t len = sizeof(MessageHeader);
	ssize_t ret;

	ret = continiousRead(pi->process->pipes[from].in, &msgHeader, len);
	if (ret <= 0) 
	{
		return -1;
	}

	len = msgHeader.s_payload_len;

	if (len > 0) 
	{
		char payload[len];

		ret = read(pi->process->pipes[from].in, &payload, len);
		if (ret == -1) 
		{
			perror("receive payload error");
			return -2;
		}

		memcpy(msg->s_payload, payload, len);
	}

	msg->s_header = msgHeader;

	return 0;
}

int receive(void *self, local_id from, Message *msg) 
{
	int ret;

	while (true) 
	{
		ret = receiveOne(self, from, msg);
		if (ret == -1) 
		{
			usleep((unsigned int)100);
			continue;
		} else if (ret == -2)
			return -1;
		else if (ret == 0)
			return 0;
	}
}

int receive_any(void *self, Message *msg) 
{
	ProcessInfo *pi = (ProcessInfo *)self;
	ssize_t returned;
	while (true) 
	{
		for (local_id i = 0; i < pi->process->count; i++) 
		{
			if (pi->process->pipes[i].in != -1) 
			{
				if ((returned = receiveOne(self, i, msg)) == -1) 
				{
					usleep((unsigned int)100);
					continue;
				} else if (returned == -2) 
				{
					return -1;
				} else if (returned == 0)
					return i;
			}
		}
		usleep((unsigned int)100);
	}
}

int receiveAll(ProcessInfo *pi, int count, MessageType type, void *buf, buff_handler bHandler) 
{
	Message msg;
	int recieved = 0;
	ssize_t returned;

	for (local_id i = 1; i <= count; i++) 
	{
		if (pi->process->pipes[i].in != -1) 
		{
			returned = receive(pi, i, &msg);
			if (returned == 0) 
			{
				if (msg.s_header.s_magic == MESSAGE_MAGIC) 
				{
					if (msg.s_header.s_type == type) 
					{
						synchronizeLamportTime(msg.s_header.s_local_time);
						if (buf != NULL && bHandler != NULL) 
						{
							bHandler(buf, &msg, recieved);
						}
						recieved++;
					} else 
						i--;
				} else 
				{
					fprintf(stderr, "corrupted magic value %d instead of %d\n", msg.s_header.s_magic, MESSAGE_MAGIC);
					return -1;
				}
			} else if (returned == -1) 
			{
				perror("parent receiveAll error");
				return -1;
			}
		}
	}
	return 0;
}
