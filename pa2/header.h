#ifndef __COMMON_HEADER__H
#define __COMMON_HEADER__H

#include "pa2345.h"
#include "banking.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char * const LogUnusedPipeReadErrorCloseFmt  = "%u Error closing unused pipe %d read %d in process %d with error: %s\n";
static const char * const LogUnusedPipeReadCloseFmt       = "%u Closing unused pipe %d read %d in process %d\n";
static const char * const LogUnusedPipeWriteErrorCloseFmt = "%u Error closing unused pipe %d write %d in process %d with error: %s\n";
static const char * const LogUnusedPipeWriteCloseFmt      = "%u Closing unused pipe %d write %d in process %d\n";
static const char * const LogUsedPipeReadErrorCloseFmt    = "%u Error closing used pipe %d read %d in process %d with error: %s\n";
static const char * const LogUsedPipeReadCloseFmt         = "%u Closing used pipe %d read %d in process %d\n";
static const char * const LogUsedPipeWriteErrorCloseFmt   = "%u Error closing used pipe %d write %d in process %d with error: %s\n";
static const char * const LogUsedPipeWriteCloseFmt        = "%u Closing used pipe %d write %d in process %d\n";
static const char * const LogPipeOpenFmt                  = "%u Open pipe from process %d to process %d: read descriptor %d, write descriptor %d\n";

typedef struct {
  int in;
  int out;
} PipeDesc;

typedef struct {
  PipeDesc *pipes;
  int count;
} PipesList;

typedef struct {
  local_id id;
  FILE *LoggingFile;
  FILE *EventsLoggingFile;
  pid_t pid;
  pid_t ppid;
  BalanceHistory *history;
  balance_t curBalance;
  PipesList *pipesList;
} Process;

typedef void (*buff_handler) (void *buff, Message *msg, int rcv);

void writeLog (FILE *stream, const char *fmt, ...);
void error (const char *msg);
int child (Process *pInfo);
int receiveAll (Process *pi, int count, MessageType type, void *buff, buff_handler bHandler);
void closeUsedPipes (Process *pInfo);
void closeUnusedPipes (Process *pInfo, PipesList *cp);
Message *createMessage (const char *payload, int payloadLength, MessageType type, timestamp_t time);


#endif // __COMMON_HEADER__H
