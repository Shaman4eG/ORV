#include <stdarg.h>
#include "header.h"

void eventLog(FILE * stream, const char * fmt, ...) 
{
	va_list args, args2;
	va_start(args, fmt);
	va_copy(args2, args);
	vfprintf(stream, fmt, args);
	vfprintf(stdout, fmt, args2);
	va_end(args);
}

int send(void * self, local_id dst, const Message * msg) 
{
	pipeDescriptor *descriptor = (pipeDescriptor*) self;
	size_t len = sizeof (MessageHeader) + msg->s_header.s_payload_len * sizeof(char);

	ssize_t returned, bytesSended = 0;
	unsigned char *buf = (unsigned char *)msg;

	while (len != 0 && (returned = write(descriptor->out, buf, len)) != 0)
	{
		if (returned == -1) 
			return -1;

		len -= returned;
		buf += returned;
		bytesSended += returned;
	}

	return 0;
}

int send_multicast(void * self, const Message * msg) 
{
	Process *descriptor = (Process*) self;
	for(int i = 0; i < descriptor->count; i++) 
	{
		if(descriptor->pipes[i].out != -1) 
		{
				if(send(&descriptor->pipes[i], i, msg) == -1) 
				{
				perror("multicast error\n");
				return -1;
				}
		}
	}

	return 0;
}

ssize_t Readsomething (int fd, void *buf, size_t len)
{
	ssize_t bytesReturned;
	size_t recieved = 0;
	unsigned char * addr = (unsigned char *)buf;
	
	while (len != 0 && (bytesReturned = read(fd , addr, len)) != 0) 
	{
		if (bytesReturned == -1 ) 
		{
			return -1;
		}

		len  -= bytesReturned;
		addr += bytesReturned;
		recieved += bytesReturned;
	}
	return recieved;
}

int receive(void * self, local_id from, Message * msg) 
{
	MessageHeader msgHeader;
	pipeDescriptor *descriptor = (pipeDescriptor*) self;
	size_t len = sizeof(MessageHeader);

	ssize_t bytesReturned = Readsomething(descriptor->in, &msgHeader, len);
	if (bytesReturned < 0)	return bytesReturned;

	len = msgHeader.s_payload_len;

	if(len > 0) 
	{
		char *payload = malloc (len * sizeof msgHeader.s_payload_len);

		bytesReturned = Readsomething(descriptor->in, payload, len);
		if ( bytesReturned < 0)	return bytesReturned;

		strcpy(msg->s_payload, payload);
		free (payload);
	}
	else 
	{
		strcpy(msg->s_payload, "");
	}
	msg->s_header = msgHeader;

	return 0;
}

int receive_any(void *self, Message *msg) 
{
	Process *process = (Process*) self;
	ssize_t bytesReturned = -1;
	for(int i = 1; i < process->count; i++) 
	{
		if(process->pipes[i].in != -1) 
		{
			if((bytesReturned = receive(&process->pipes[i], i, msg)) == -1) 
				return -1;
			else if (bytesReturned == 0) break;
		}
	}

	if (bytesReturned != 0) return -2;

	return 0;
}

int receiveAll(Process *process, int count, MessageType msgType, int *anotherCnt) 
{
	Message msg;
	int recieved = 0;
	int anotherRecieve = 0;

	for(int i = 1; i < process->count && recieved < count; i++)
	{
		if (process->pipes[i].in != -1) 
		{
			if (receive(&process->pipes[i], i, &msg) == 0) 
			{
				if (msg.s_header.s_magic == MESSAGE_MAGIC)
				{
					if (msg.s_header.s_type == msgType)
						recieved++;
					else {
						anotherRecieve++;
						fprintf(stderr, "unhandled message type %d\n", msg.s_header.s_type);
					}
				}
			}
			else {
				perror("parent receive_any error");
			}
		}
	}

	*anotherCnt = anotherRecieve;
	return 0;
}
