#include <sys/wait.h>
#include <fcntl.h>
#include "header.h"

void error(const char *msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

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

	childPipe* procPipes = malloc(procCount * sizeof(childPipe));
	for (size_t i = 0; i < procCount; i++)
	{
		procPipes[i].pipes = malloc(procCount * sizeof(pipeDesc));
		procPipes[i].count = procCount;
	}

	int n = 0;
	int k = n + 1;
	int inFD[2], outFD[2];
	//size_t base = pipesCount - (procCount - 1);
	printf("pipesCount %zu\n", pipesCount);
	for (int i = 0; i < pipesCount; i++) {

		if (pipe(inFD) < 0 || pipe(outFD) < 0)
			error("child pipe call error");

		//fcntl(inFD[i][0], F_SETFL, O_NONBLOCK);
		//fcntl(inFD[i][1], F_SETFL, O_NONBLOCK);
		//fcntl(outFD[i][0], F_SETFL, O_NONBLOCK);
		//fcntl(outFD[i][1], F_SETFL, O_NONBLOCK);
		//0 - read
		//1 - write
		procPipes[n].pipes[k].in = inFD[0];
		procPipes[n].pipes[k].out = outFD[1];
		procPipes[k].pipes[n].in = outFD[0];
		procPipes[k].pipes[n].out = inFD[1];
		printf("%d in %d to %d out %d\n", n, inFD[0], k, outFD[1]);
		printf("%d in %d to %d out %d\n", k, outFD[0], n, inFD[1]);

		fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), n, k, outFD[0], outFD[1]);
		fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), k, n, inFD[0], inFD[1]);

		k++;

		if (k >= procCount) {
			n++;
			k = n + 1;
		}
	}

	//for(int i = 0; i < procCount - 1; i++) {
	//  if(pipe(inFD[base + i]) < 0)
	//      error("parent pipe call error");
	//  procPipes[PARENT_ID].pipes[i + 1].in  = inFD[base + i][0];
	//  procPipes[i + 1].pipes[PARENT_ID].out = inFD[base + i][1];
	//  procPipes[i + 1].pipes[PARENT_ID].in  = -1;
	//  procPipes[PARENT_ID].pipes[i + 1].out = -1;

	//  fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), PARENT_ID, i + 1, inFD[base + i][0], inFD[base + i][1]);
	//}

	for (int i = 0; i < procCount; i++) {
		procPipes[i].pipes[i].in = -1;
		procPipes[i].pipes[i].out = -1;
	}
	//=============================================================

	pid_t pid;
	for (int i = 1; i < procCount; i++) 
	{
		//printf("loop iteration %d\n", i);
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
