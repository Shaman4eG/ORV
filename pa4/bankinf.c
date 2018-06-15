#include "header.h"
#include <stdbool.h>
#define d 1

static timestamp_t localTime = 0;

void transfer(void *parent_data, local_id source, local_id destination, balance_t amount) 
{
	TransferOrder tOrder = {
		.s_src = source,
		.s_dst = destination,
		.s_amount = amount
	};

	noteEventForLamportTime();
	const Message *msg = createMessage((char *) &tOrder, sizeof (TransferOrder), TRANSFER, get_lamport_time());
	send(parent_data, source, msg);
	free((char*)msg);

	Message received;
	receive(parent_data, destination, &received);

	if (received.s_header.s_magic != MESSAGE_MAGIC && received.s_header.s_type != ACK)
	{
		fprintf(stderr, "tranfer error\n");
	}
 
	synchronizeLamportTime(received.s_header.s_local_time);
}

void synchronizeLamportTime(timestamp_t time)
{
    if (time > localTime)
        localTime = time;
	noteEventForLamportTime();
}

void noteEventForLamportTime() 
{
	localTime = localTime + 1;
}

timestamp_t get_lamport_time() 
{
    return localTime;
}
