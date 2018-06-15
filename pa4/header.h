#ifndef __COMMON_HEADER__H
#define __COMMON_HEADER__H

#include "pa2345.h"
#include "banking.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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
} PipeDescriptor;

typedef struct {
  PipeDescriptor *pipes;
  int count;
} Process;

typedef struct {
  bool isMutex;
  Process *process;
  int workers;
  local_id id;
  pid_t pid;
  pid_t ppid;
  balance_t curBalance;
  FILE *fPipesLog;
  FILE *fEventsLog;
} ProcessInfo;

typedef void (*buff_handler) (void *buff, Message *msg, int rcv);

void writeLog (FILE *stream, const char *fmt, ...);
void error (const char *msg);
int child (ProcessInfo *pInfo);
int receiveAll (ProcessInfo *pi, int count, MessageType type, void *buff, buff_handler bHandler);
void closeUsedPipes (ProcessInfo *pInfo);
void closeUnusedPipes (ProcessInfo *pInfo, Process *cp);
Message *createMessage (const char *payload, int payloadLength, MessageType type, timestamp_t time);
void synchronizeLamportTime(timestamp_t time);
void noteEventForLamportTime();

#endif // __COMMON_HEADER__H
