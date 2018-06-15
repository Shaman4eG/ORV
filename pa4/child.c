#include <sys/stat.h>
#include "header.h"

#define MAX_LOOP_STR 1024

int sendDone (ProcessInfo *pInfo) 
{
	noteEventForLamportTime();
	timestamp_t time = get_lamport_time();
	char messageDone[MAX_PAYLOAD_LEN];
	int length = sprintf(messageDone, log_done_fmt, time, pInfo->id, pInfo->curBalance);
	if(length < 0) 
	{
		perror("sprintf done error");
		return -1;
	}

	const Message *msg = createMessage(messageDone, length, DONE, time);

	writeLog(pInfo->fEventsLog, log_done_fmt, time, pInfo->id, pInfo->curBalance);
	if(send_multicast(pInfo, msg) == -1) 
	{
		perror("send done error");
		free((char *)msg);
		return -1;
	}
	free((char*)msg);
	return 0;
}

int sendStart (ProcessInfo *pInfo, timestamp_t time) 
{
	char messageStarted[MAX_PAYLOAD_LEN];
	int length = sprintf(messageStarted, log_started_fmt, time, pInfo->id, pInfo->pid, pInfo->ppid, pInfo->curBalance);
	if (length < 0) 
	{
		perror("sprintf error");
		return -1;
	}
	const Message *msg = createMessage(messageStarted, length, STARTED, time);

	writeLog (pInfo->fEventsLog, log_started_fmt, time, pInfo->id, pInfo->pid, pInfo->ppid, pInfo->curBalance);
	if(send_multicast(pInfo, msg) == -1) 
	{
		fprintf(stderr, "[child %d] send_multicast error: %s", pInfo->id, strerror(errno));
		free((char *)msg);
		return -1;
	}

	free((char*)msg);
	return 0;
}

int loopPrint(ProcessInfo *pInfo) 
{
	if (pInfo->isMutex)
		request_cs(pInfo);

	char buf[MAX_LOOP_STR];
	uint8_t loopCount = pInfo->id * 5;

	for ( uint8_t i = 1; i <= loopCount; i++ ) 
	{
		snprintf (buf, sizeof buf, log_loop_operation_fmt, pInfo->id, i, loopCount);
		print (buf);
	}

	if (pInfo->isMutex)
		release_cs(pInfo);

	return 0;
}

int child (ProcessInfo *pInfo) 
{
	pInfo->pid = getpid();
	pInfo->ppid = getppid();

	noteEventForLamportTime();
	if (sendStart(pInfo, get_lamport_time()) == -1)
		return -1;
	if (receiveAll(pInfo, pInfo->process->count - 1, STARTED, NULL, NULL) == -1)
		return -1;
	writeLog(pInfo->fEventsLog, log_received_all_started_fmt, get_lamport_time(), pInfo->id);

	loopPrint(pInfo);

	if (sendDone(pInfo) == -1)
		return -1;
	pInfo->workers--;

	Message msg;
	while (pInfo->workers > 0) 
	{
		if (receive_any(pInfo, &msg) == -1) 
		{
			fprintf(stderr, "[id: %d] child receive_any error\n",pInfo->id);
			return -1;
		}
		synchronizeLamportTime(msg.s_header.s_local_time);
		if (msg.s_header.s_magic == MESSAGE_MAGIC)
			if (msg.s_header.s_type == DONE) 
			{
				pInfo->workers--;
			}
	}

	writeLog(pInfo->fEventsLog, log_received_all_done_fmt, get_lamport_time(), pInfo->id);
	return 0;
}
