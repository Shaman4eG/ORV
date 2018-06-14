#include <sys/wait.h>
#include <fcntl.h>
#include "header.h"

void realCloseUnusedPipes(childPipe *cp, int i, int j, FILE *flog)
{
	if (cp[i].pipes[j].out != -1)
	{
		if (close(cp[i].pipes[j].out) == -1)
			fprintf(flog, LogUnusedPipeWriteErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i, strerror(errno));
		else
			fprintf(flog, LogUnusedPipeWriteCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i);
	}
	if (cp[i].pipes[j].in != -1)
	{
		if (close(cp[i].pipes[j].in) == -1)
			fprintf(flog, LogUnusedPipeReadErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j, strerror(errno));
		else
			fprintf(flog, LogUnusedPipeReadCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j);
	}
}

void freeUnusedPipesMemory(int count, childPipe *cp, int id)
{
	for (int i =  1; i < count; i++)
	{
		if (i == id) continue;
		else free(cp[i].pipes);
	}
}

void closeUnsuedPipes(int id, int count, FILE *flog, childPipe *cp)
{
	for (int i = 0; i < count; i++) 
	{
		if (i == id) continue;
		else
		{
			for (int j = 0; j < cp[i].count; j++)
			{
				realCloseUnusedPipes(cp, i, j, flog);
			}
		}
	}

	freeUnusedPipesMemory(count, cp, id);
}

void closeUsedPipesIn(FILE *flog, int id, int j, childPipe *cp)
{
	if (cp->pipes[j].in != -1)
	{
		if (close(cp->pipes[j].in) == -1)
			fprintf(flog, LogUsedPipeReadErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id, strerror(errno));
		else
			fprintf(flog, LogUsedPipeReadCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id);
	}
}

void closeUsedPipesOut(FILE *flog, int id, int j, childPipe *cp)
{
	if (cp->pipes[j].out != -1)
	{
		if (close(cp->pipes[j].out) == -1)
			fprintf(flog, LogUsedPipeWriteErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id, strerror(errno));
		else
			fprintf(flog, LogUsedPipeWriteCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id);
	}
}

void closeUsedPipes(int id, childPipe *cp, FILE *flog)
{
    for (int j = 0; j < cp->count; j++)
	{
		closeUsedPipesIn(flog, PARENT_ID, j, cp);
		closeUsedPipesOut(flog, PARENT_ID, j, cp);
    }

    free(cp->pipes);
}

void doForks(childPipe *procPipes, int procCount, FILE *flog, FILE *ef)
{
	pid_t pid;
	for (int i = 1; i < procCount; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			closeUnsuedPipes(i, procCount, flog, procPipes);
			child(i, &procPipes[i], flog, ef);
			exit(EXIT_SUCCESS);
		}
	}
}

void doForkWithExtra(childPipe *procPipes, int procCount, FILE *flog, FILE *ef)
{
	doForks(procPipes, procCount, flog, ef);
	closeUnsuedPipes(PARENT_ID, procCount, flog, procPipes);
}

void createPipes(size_t pipesCount, childPipe* procPipes, FILE *flog, size_t procCount)
{
	int inFD[2];
	int outFD[2];

	for (int fromProcess = 0; fromProcess < procCount; fromProcess++)
	{
		for (int toProcess = fromProcess + 1; toProcess <= procCount; toProcess++)
		{
			pipe(inFD);
			pipe(outFD);

			procPipes[toProcess].pipes[fromProcess].in = outFD[0];
			procPipes[toProcess].pipes[fromProcess].out = inFD[1];
			procPipes[fromProcess].pipes[toProcess].in = inFD[0];
			procPipes[fromProcess].pipes[toProcess].out = outFD[1];

			fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), fromProcess, toProcess, outFD[0], outFD[1]);
			fprintf(flog, LogPipeOpenFmt, (unsigned)time(NULL), toProcess, n, inFD[0], inFD[1]);
		}
	}
}

void recieveAllAndLog(childPipe* procPipes, size_t procCount, FILE *ef)
{
	int rcvDone = 0;

	receiveAll(&procPipes[0], procCount - 1, STARTED, &rcvDone);
	eventLog(ef, log_received_all_started_fmt, PARENT_ID);

	receiveAll(&procPipes[0], procCount - 1 - rcvDone, DONE, &rcvDone);
	eventLog(ef, log_received_all_done_fmt, PARENT_ID);
}

void waitFinishing(size_t procCount)
{
	int finished = 0;
	pid_t wpid;
	while (finished < procCount && (wpid = wait(NULL)) > 0)
	{
		finished++;
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

	for (size_t i = 0; i < procCount; i++)
	{
		procPipes[i].count = procCount;
		procPipes[i].pipes = malloc(procCount * sizeof(pipeDesc));
	}

	createPipes(pipesCount, procPipes, flog, procCount);

	for (int i = 0; i < procCount; i++) 
	{
		procPipes[i].pipes[i].out = -1;
		procPipes[i].pipes[i].in = -1;
	}
	//=============================================================

	doForkWithExtra(procPipes, procCount, flog, ef);

	recieveAllAndLog(procPipes, procCount, ef);

	closeUsedPipes(PARENT_ID, &procPipes[0], flog);

	waitFinishing(procCount);

	fclose(flog);
	fclose(ef);

	exit(EXIT_SUCCESS);
}
