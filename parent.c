#include <sys/wait.h>
#include <fcntl.h>
#include "header.h"

void closeUnsuedPipes(FILE *flog, childPipe *cp, int count, int id) {
  for(int i = 0; i < count; i++) {//TODO another function
    if(i != id) {
      for(int j = 0; j < cp[i].count; j++){
        if (cp[i].pipes[j].in != -1) {
          if(close(cp[i].pipes[j].in) == -1)
            fprintf(flog, LogUnusedPipeReadErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j, strerror(errno));
          else
            fprintf(flog, LogUnusedPipeReadCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j);
        }

        if (cp[i].pipes[j].out != -1) {
          if(close(cp[i].pipes[j].out) == -1)
            fprintf(flog, LogUnusedPipeWriteErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i, strerror(errno));
          else
            fprintf(flog, LogUnusedPipeWriteCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i);
        }
      }
    }
  }

  //for (int j = 0; j < count; j++) {
  //  if (cp[PARENT_ID].pipes[j].out != -1) {
  //    if(close(cp[PARENT_ID].pipes[j].out) == -1)
  //      fprintf(flog, LogUnusedPipeWriteErrorCloseFmt, (unsigned)time(NULL), PARENT_ID, cp[PARENT_ID].pipes[j].out, j, strerror(errno));
  //    else
  //      fprintf(flog, LogUnusedPipeWriteCloseFmt, (unsigned)time(NULL), PARENT_ID, cp[PARENT_ID].pipes[j].out, j);
  //  }

  //}


  for(int i = 1; i < count; i++) {
    if (i != id)
      free(cp[i].pipes);
  }
}

void closeUsedPipes(FILE *flog, childPipe *cp, int id) {
    for(int j = 0; j < cp->count; j++){
      if (cp->pipes[j].in != -1) {
        if(close(cp->pipes[j].in) == -1)
          fprintf(flog, LogUsedPipeReadErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id, strerror(errno));
        else
          fprintf(flog, LogUsedPipeReadCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id);
      }

      if (cp->pipes[j].out != -1) {
        if(close(cp->pipes[j].out) == -1)
          fprintf(flog, LogUsedPipeWriteErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id, strerror(errno));
        else
          fprintf(flog, LogUsedPipeWriteCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id);
      }
    }

    free(cp->pipes);
}


void createPipes(size_t pipesCount, childPipe* procPipes, FILE *flog, size_t procCount)
{
	int inFD[2];
	int outFD[2];
	int n = 0;
	int k = 1;

	for (int i = pipesCount - 1; i >= 0; i--)
	{
		pipe(inFD);
		pipe(outFD);

		procPipes[k].pipes[n].in = outFD[0];
		procPipes[k].pipes[n].out = inFD[1];
		procPipes[n].pipes[k].in = inFD[0];
		procPipes[n].pipes[k].out = outFD[1];

		fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), n, k, outFD[0], outFD[1]);
		fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), k, n, inFD[0], inFD[1]);

		k++;

		if (k >= procCount)
		{
			n++;
			k = n + 1;
		}
	}
}

int main(int argc, char *argv[]) {

	size_t procCount = atoi(argv[2]) + 1;

	FILE *ef = fopen(events_log, "a");
	FILE *flog = fopen(pipes_log, "a");

	//=============================================================
	size_t pipesCount = 0;

	for (size_t i = 1; i < procCount; i++)
	{
		pipesCount += i;
	}
	
	childPipe* procPipes = malloc(sizeof(childPipe) * procCount);
	for (size_t i = procCount - 1; i >= 0; i--)
	{
		procPipes[i].count = procCount;
		procPipes[i].pipes = malloc(procCount * sizeof(pipeDesc));
	}

	createPipes(pipesCount, procPipes, flog, procCount);

	for (int i = procCount - 1; i >= 0; i--) 
	{
		procPipes[i].pipes[i].out = -1;
		procPipes[i].pipes[i].in = -1;
	}
	//=============================================================

	pid_t pid;
	for (int i = 1; i < procCount; i++) 
	{
		pid = fork ();
		if (pid == 0) 
		{
			closeUnsuedPipes(flog, procPipes, procCount, i);
			if (child(i, &procPipes[i], flog, ef) == -1)
			error("child");
			else
			exit(EXIT_SUCCESS);
		}
		else if (pid == -1)
			error("fork error");
	}

	closeUnsuedPipes(flog, procPipes, procCount, PARENT_ID);
	int rcvDone = 0;

	if(receiveAll(&procPipes[0], procCount - 1, STARTED, &rcvDone) == -1)
	error("receiveAll error");
	eventLog(ef, log_received_all_started_fmt, PARENT_ID);


	if(receiveAll(&procPipes[0], procCount - 1 - rcvDone, DONE, &rcvDone) == -1)
	error("receiveAll DONE error");
	eventLog(ef, log_received_all_done_fmt, PARENT_ID);



	int finished = 0;
	pid_t wpid;
	while(finished<procCount && (wpid = wait(NULL)) > 0)
	{
	finished++;
	}


	closeUsedPipes(flog, &procPipes[0], PARENT_ID);


	fclose(ef);
	fclose(flog);

	exit(EXIT_SUCCESS);
}
