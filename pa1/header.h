#ifndef __HEADER__H
#define __HEADER__H

#include "ipc.h"
#include "common.h"
#include "pa1.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void closeUsedPipes(int id, Process *cp, FILE *LoggingFile);
void closeUnusedPipes(int id, int count, FILE *LoggingFile, Process *cp);
void eventLog(FILE * stream, const char *fmt, ...);
int child (int id, Process *pipeList, FILE *LoggingFile, FILE *EventsLoggingFile);
int receiveAll(Process *cp, int count, MessageType type, int *anotherCnt);

static const char * const UsedPipeReadCloseFmtLog       = "%u Closing used pipe %d read %d in process %d\n";
static const char * const UsedPipeWriteErrorCloseFmtLog = "%u Error closing used pipe %d write %d in process %d with error: %s\n";
static const char * const UsedPipeWriteCloseFmtLog      = "%u Closing used pipe %d write %d in process %d\n";
static const char * const PipeOpenFmtLog            = "%u Open pipe from process %d to process %d: read descriptor %d, write descriptor %d\n";
static const char * const UnusedPipeReadErrorCloseFmtLog  = "%u Error closing unused pipe %d read %d in process %d with error: %s\n";
static const char * const UnusedPipeReadCloseFmtLog       = "%u Closing unused pipe %d read %d in process %d\n";
static const char * const UnusedPipeWriteErrorCloseFmtLog = "%u Error closing unused pipe %d write %d in process %d with error: %s\n";
static const char * const UnusedPipeWriteCloseFmtLog      = "%u Closing unused pipe %d write %d in process %d\n";
static const char * const UsedPipeReadErrorCloseFmtLog  = "%u Error closing used pipe %d read %d in process %d with error: %s\n";

typedef struct {
  int in;
  int out;
} pipeDescriptor;

typedef struct {
  pipeDescriptor *pipes;
  int count;
} Process;

#endif
