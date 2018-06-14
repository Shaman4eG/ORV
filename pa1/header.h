#ifndef __COMMON_HEADER__H
#define __COMMON_HEADER__H

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

static const char * const LogUnusedPipeReadErrorCloseFmt  = "%u Error closing unused pipe %d read %d in process %d with error: %s\n";
static const char * const LogUnusedPipeReadCloseFmt       = "%u Closing unused pipe %d read %d in process %d\n";
static const char * const LogUnusedPipeWriteErrorCloseFmt = "%u Error closing unused pipe %d write %d in process %d with error: %s\n";
static const char * const LogUnusedPipeWriteCloseFmt      = "%u Closing unused pipe %d write %d in process %d\n";
static const char * const LogUsedPipeReadErrorCloseFmt  = "%u Error closing used pipe %d read %d in process %d with error: %s\n";
static const char * const LogUsedPipeReadCloseFmt       = "%u Closing used pipe %d read %d in process %d\n";
static const char * const LogUsedPipeWriteErrorCloseFmt = "%u Error closing used pipe %d write %d in process %d with error: %s\n";
static const char * const LogUsedPipeWriteCloseFmt      = "%u Closing used pipe %d write %d in process %d\n";
static const char * const LogPipeOpenFmt            = "%u Open pipe from process %d to process %d: read descriptor %d, write descriptor %d\n";

typedef struct {
  int in;
  int out;
} pipeDesc;

typedef struct {
  pipeDesc *pipes;
  size_t count;
} childPipe;

void eventLog(FILE * stream, const char *fmt, ...);
int child (int id, childPipe *pipeList, FILE *flog, FILE *ef);
int receiveAll(childPipe *cp, int count, MessageType type, int *anotherCnt);
void closeUsedPipes(int id, childPipe *cp, FILE *flog);
void closeUnsuedPipes(int id, int count, FILE *flog, childPipe *cp);

#endif // __COMMON_HEADER__H
