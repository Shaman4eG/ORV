#include <sys/wait.h>
#include <fcntl.h>
#include "header.h"

void realCloseUnusedPipes(Process *cp, int i, int j, FILE *LoggingFile)
{
	if (cp[i].pipes[j].out != -1)
	{
		if (close(cp[i].pipes[j].out) == -1)
			fprintf(LoggingFile, LogUnusedPipeWriteErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i, strerror(errno));
		else
			fprintf(LoggingFile, LogUnusedPipeWriteCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].out, i);
	}
	if (cp[i].pipes[j].in != -1)
	{
		if (close(cp[i].pipes[j].in) == -1)
			fprintf(LoggingFile, LogUnusedPipeReadErrorCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j, strerror(errno));
		else
			fprintf(LoggingFile, LogUnusedPipeReadCloseFmt, (unsigned)time(NULL), i, cp[i].pipes[j].in, j);
	}
}

void freeUnusedPipesMemory(int count, Process *cp, int id)
{
	for (int i =  1; i < count; i++)
	{
		if (i == id) continue;
		else free(cp[i].pipes);
	}
}

void closeUnusedPipes(int id, int count, FILE *LoggingFile, Process *cp)
{
	for (int i = 0; i < count; i++) 
	{
		if (i == id) continue;
		else
		{
			for (int j = 0; j < cp[i].count; j++)
			{
				realCloseUnusedPipes(cp, i, j, LoggingFile);
			}
		}
	}

	freeUnusedPipesMemory(count, cp, id);
}

void closeUsedPipesIn(FILE *LoggingFile, int id, int j, Process *cp)
{
	if (cp->pipes[j].in != -1)
	{
		if (close(cp->pipes[j].in) == -1)
			fprintf(LoggingFile, LogUsedPipeReadErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id, strerror(errno));
		else
			fprintf(LoggingFile, LogUsedPipeReadCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].in, id);
	}
}

void closeUsedPipesOut(FILE *LoggingFile, int id, int j, Process *cp)
{
	if (cp->pipes[j].out != -1)
	{
		if (close(cp->pipes[j].out) == -1)
			fprintf(LoggingFile, LogUsedPipeWriteErrorCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id, strerror(errno));
		else
			fprintf(LoggingFile, LogUsedPipeWriteCloseFmt, (unsigned)time(NULL), j, cp->pipes[j].out, id);
	}
}

void closeUsedPipes(int id, Process *cp, FILE *LoggingFile)
{
    for (int j = 0; j < cp->count; j++)
	{
		closeUsedPipesIn(LoggingFile, PARENT_ID, j, cp);
		closeUsedPipesOut(LoggingFile, PARENT_ID, j, cp);
    }

    free(cp->pipes);
}

void doForks(Process *procPipes, int numberOfProcesses, FILE *LoggingFile, FILE *EventsLoggingFile)
{
	pid_t pid;
	for (int i = 1; i < numberOfProcesses; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			closeUnusedPipes(i, numberOfProcesses, LoggingFile, procPipes);
			child(i, &procPipes[i], LoggingFile, EventsLoggingFile);
			exit(EXIT_SUCCESS);
		}
	}
}

void doForkWithExtra(Process *procPipes, int numberOfProcesses, FILE *LoggingFile, FILE *EventsLoggingFile)
{
	doForks(procPipes, numberOfProcesses, LoggingFile, EventsLoggingFile);
	closeUnusedPipes(PARENT_ID, numberOfProcesses, LoggingFile, procPipes);
}

void createPipes(int pipesCount, Process* procPipes, FILE *LoggingFile, int numberOfProcesses)
{
	int inFD[2];
	int outFD[2];

	for (int fromProcess = 0; fromProcess < numberOfProcesses; fromProcess++)
	{
		for (int toProcess = fromProcess + 1; toProcess <= numberOfProcesses; toProcess++)
		{
			pipe(inFD);
			pipe(outFD);

			procPipes[toProcess].pipes[fromProcess].in = outFD[0];
			procPipes[toProcess].pipes[fromProcess].out = inFD[1];
			procPipes[fromProcess].pipes[toProcess].in = inFD[0];
			procPipes[fromProcess].pipes[toProcess].out = outFD[1];

			fprintf(LoggingFile, LogPipeOpenFmt, (unsigned)time(NULL), fromProcess, toProcess, outFD[0], outFD[1]);
			fprintf(LoggingFile, LogPipeOpenFmt, (unsigned)time(NULL), toProcess, fromProcess, inFD[0], inFD[1]);
		}
	}
}

void recieveAllAndLog(Process* procPipes, int numberOfProcesses, FILE *EventsLoggingFile)
{
	int rcvDone = 0;

	receiveAll(&procPipes[0], numberOfProcesses - 1, STARTED, &rcvDone);
	eventLog(EventsLoggingFile, log_received_all_started_fmt, PARENT_ID);

	receiveAll(&procPipes[0], numberOfProcesses - 1 - rcvDone, DONE, &rcvDone);
	eventLog(EventsLoggingFile, log_received_all_done_fmt, PARENT_ID);
}

void waitFinishing(int numberOfProcesses)
{
	int finished = 0;
	pid_t wpid;
	while (finished < numberOfProcesses && (wpid = wait(NULL)) > 0)
	{
		finished++;
	}
}

int main(int argc, char *argv[]) {
	int numberOfProcesses = atoi(argv[2]) + 1;

	FILE *EventsLoggingFile = fopen(events_log, "a");
	FILE *LoggingFile = fopen(pipes_log, "a");

	//=============================================================
	int numberOfPipes = 0;

	for (int i = 1; i < numberOfProcesses; i++)
		numberOfPipes += i;
	
	Process* processes = malloc(sizeof(Process) * numberOfProcesses);

	for (int i = 0; i < numberOfProcesses; i++)
	{
		processes[i].count = numberOfProcesses;
		processes[i].pipes = malloc(numberOfProcesses * sizeof(pipeDescriptor));
	}

	createPipes(numberOfPipes, processes, LoggingFile, numberOfProcesses);

	for (int i = 0; i < numberOfProcesses; i++) 
	{
		processes[i].pipes[i].out = -1;
		processes[i].pipes[i].in = -1;
	}
	//=============================================================

	doForkWithExtra(processes, numberOfProcesses, LoggingFile, EventsLoggingFile);

	recieveAllAndLog(processes, numberOfProcesses, EventsLoggingFile);

	closeUsedPipes(PARENT_ID, &processes[0], LoggingFile);

	waitFinishing(numberOfProcesses);

	fclose(LoggingFile);
	fclose(EventsLoggingFile);

	exit(EXIT_SUCCESS);
}
