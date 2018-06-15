#include "header.h"
#include "queue.h"

int request_cs (const void *self) 
{
	ProcessInfo *pi = (ProcessInfo *)self;
	noteEventForLamportTime();
	Message *msg = createMessage (NULL, 0, CS_REQUEST, get_lamport_time());
	if(send_multicast (pi, msg) == 1)
	{
		fprintf(stderr, "[id: %d] request_cs send_multicast error\n", pi->id);
		return -1;
	}

	initQueue (pi->process->count);

	Node *node = malloc (sizeof (Node));
	node->id = pi->id;
	node->time = get_lamport_time();

	enqueue (node);

	int replied = 0;
	local_id sender;
	Node *minNode;

	while (replied < (pi->workers - 1) || ((minNode = queueFont()) && minNode->id != pi->id)) 
	{
		if ((sender = receive_any (pi, msg)) == -1) 
		{
			fprintf(stderr, "[id: %d] request_cs receive_any  error\n", pi->id);
			return -1;
		};

		synchronizeLamportTime (msg->s_header.s_local_time);

		switch (msg->s_header.s_type) 
		{
			case CS_REQUEST:
				node = malloc (sizeof (Node));
				node->id = sender;
				node->time = msg->s_header.s_local_time;
				enqueue(node);
				noteEventForLamportTime();
				msg->s_header.s_type = CS_REPLY;
				msg->s_header.s_local_time = get_lamport_time();
				if (send(pi, sender, msg) == -1)
				{
					fprintf(stderr, "[id: %d] request_cs send error\n", pi->id);
					return -1;
				}
				break;

			case CS_REPLY:
				replied++;
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

	return 0;
}

int release_cs (const void *self) 
{
	ProcessInfo *pi = (ProcessInfo *)self;

	dequeue();

	noteEventForLamportTime();
	Message *msg = createMessage( NULL, 0, CS_RELEASE, get_lamport_time());
	if (send_multicast(pi, msg) == -1) 
	{
		fprintf(stderr, "[id: %d] request_cs send_multicast error\n", pi->id);
		return -1;
	}
	return 0;
}
