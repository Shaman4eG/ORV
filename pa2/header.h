#ifndef __HEADER__H
#define __HEADER__H

#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "pa2345.h"
#include "banking.h"

static const char * const UnusedPipeReadErrorCloseFmtLog  = "%u Error closing unused pipe %d read %d in process %d with error: %s\n";
static const char * const UnusedPipeReadCloseFmtLog       = "%u Closing unused pipe %d read %d in process %d\n";
static const char * const UnusedPipeWriteErrorCloseFmtLog = "%u Error closing unused pipe %d write %d in process %d with error: %s\n";
static const char * const UnusedPipeWriteCloseFmtLog      = "%u Closing unused pipe %d write %d in process %d\n";
static const char * const UsedPipeReadErrorCloseFmtLog    = "%u Error closing used pipe %d read %d in process %d with error: %s\n";
static const char * const UsedPipeReadCloseFmtLog         = "%u Closing used pipe %d read %d in process %d\n";
static const char * const UsedPipeWriteErrorCloseFmtLog   = "%u Error closing used pipe %d write %d in process %d with error: %s\n";
static const char * const UsedPipeWriteCloseFmtLog        = "%u Closing used pipe %d write %d in process %d\n";
static const char * const PipeOpenFmtLog                  = "%u Open pipe from process %d to process %d: read descriptor %d, write descriptor %d\n";

typedef struct {
	int in;
	int out;
} PipeDescriptor;

typedef struct {
	int count;
	PipeDescriptor *pipes;
} ArrayOfPipes;

typedef struct {
	local_id id;
	FILE *LoggingFile;
	FILE *EventsLoggingFile;
	BalanceHistory *history;
	balance_t currentBalance;
	pid_t pid;
	pid_t ppid;
	ArrayOfPipes *arrayOfPipes;
} Process;

typedef void (*handlerForBuffer) (void *buffer, Message *msg, int rcv);

void closeUsedPipes(Process *childProcess);
void closeUnusedPipes(ArrayOfPipes *processesPipes, Process *parentProcess);
void writeLog (FILE *stream, const char *fmt, ...);
void error (const char *msg);
int child (Process *pInfo);
int receiveAll (Process *pi, int count, MessageType type, void *buff, handlerForBuffer bHandler);
Message *createMessage (const char *payload, int payloadLength, MessageType type, timestamp_t time);


#endif
