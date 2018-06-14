#include <sys/stat.h>
#include <errno.h>
#include "header.h"

Message *generateMsg(MessageType msgType, const char* payload, int payloadLength)
{
	  Message *msg = malloc(sizeof (Message));
	  msg->s_header.s_type = msgType;
	  msg->s_header.s_local_time = (timestamp_t) time(NULL);
	  msg->s_header.s_magic = MESSAGE_MAGIC;
	  msg->s_header.s_payload_len = payloadLength;
	  strcpy(msg->s_payload, payload);
	  return msg;
}

int CheckStartedMsg(int id, pid_t pid, pid_t parentPid, Process *process, FILE *EventsLoggingFile)
{
	int rcvDone = 0;
	char messageStarted[49];
	int length = sprintf(messageStarted, log_started_fmt, id, pid, parentPid);
	if (length < 0)	return -1;
	
	const Message *msg = generateMsg(STARTED, messageStarted, length);

	// Sending
	if (send_multicast(process, msg) == -1) 
		return -1;

	eventLog(EventsLoggingFile, log_started_fmt, id, pid, parentPid);

	free((char*)msg);

	// Recieving
	if (receiveAll(process, process->count - 2, STARTED, &rcvDone) == -1) 
		return -1;

	eventLog(EventsLoggingFile, log_received_all_started_fmt, id);

	return 0;
}

int CheckDoneMsg(int id, Process *process, FILE *EventsLoggingFile)
{
	int rcvDone = 0;
	char messageDone[29];
	int length = sprintf(messageDone, log_done_fmt, id);
	if (length < 0) 
		return -1;

	const Message *msg = generateMsg(DONE, messageDone, length);

	//sending
	if (send_multicast(process, msg) == -1) 
		return -1;
	
	eventLog(EventsLoggingFile, log_done_fmt, id);

	//recieving
	if (receiveAll(process, process->count - 2 - rcvDone, DONE, &rcvDone) == -1) 
		return -1;

	eventLog(EventsLoggingFile, log_received_all_done_fmt, id);
	
	free((char*)msg);
}

int child(int id, Process *process, FILE *LoggingFile, FILE *EventsLoggingFile) 
{
	if (EventsLoggingFile == NULL) 
		return -1;
	
	pid_t pid = getpid();
	pid_t parentPid = getppid();

	if (CheckStartedMsg(id, pid, parentPid, process, EventsLoggingFile) == -1)
		return -1;

	if (CheckDoneMsg(id, process, EventsLoggingFile) == -1)
		return -1;

	closeUsedPipes(id, process, LoggingFile);
	fclose(EventsLoggingFile);
	fclose(LoggingFile);
	return 0;
}
