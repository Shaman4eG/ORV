#include <sys/wait.h>
#include <fcntl.h>
#include "header.h"

void realCloseUnusedPipes(Process *processes, int k, int j, FILE *LoggingFile)
{
	if (processes[k].pipes[j].out != -1)
	{
		if (close(processes[k].pipes[j].out) == -1)
			fprintf(LoggingFile, UnusedPipeWriteErrorCloseFmtLog, (unsigned)time(NULL), k, processes[k].pipes[j].out, k, strerror(errno));
		else
			fprintf(LoggingFile, UnusedPipeWriteCloseFmtLog, (unsigned)time(NULL), k, processes[k].pipes[j].out, k);
	}
	if (processes[k].pipes[j].in != -1)
	{
		if (close(processes[k].pipes[j].in) == -1)
			fprintf(LoggingFile, UnusedPipeReadErrorCloseFmtLog, (unsigned)time(NULL), k, processes[k].pipes[j].in, j, strerror(errno));
		else
			fprintf(LoggingFile, UnusedPipeReadCloseFmtLog, (unsigned)time(NULL), k, processes[k].pipes[j].in, j);
	}
}

void freeUnusedPipesMemory(int count, Process *processes, int id)
{
	for (int i =  1; i < count; i++)
	{
		if (i == id) continue;
		else free(processes[i].pipes);
	}
}

void closeUnusedPipes(int id, int count, FILE *LoggingFile, Process *processes)
{
	for (int i = 0; i < count; i++) 
	{
		if (i == id) continue;
		else
		{
			for (int j = 0; j < processes[i].count; j++)
			{
				realCloseUnusedPipes(processes, i, j, LoggingFile);
			}
		}
	}

	freeUnusedPipesMemory(count, processes, id);
}

void closeUsedPipesIn(FILE *LoggingFile, int id, int j, Process *processes)
{
	if (processes->pipes[j].in != -1)
	{
		if (close(processes->pipes[j].in) == -1)
			fprintf(LoggingFile, UsedPipeReadErrorCloseFmtLog, (unsigned)time(NULL), j, processes->pipes[j].in, id, strerror(errno));
		else
			fprintf(LoggingFile, UsedPipeReadCloseFmtLog, (unsigned)time(NULL), j, processes->pipes[j].in, id);
	}
}

void closeUsedPipesOut(FILE *LoggingFile, int id, int j, Process *processes)
{
	if (processes->pipes[j].out != -1)
	{
		if (close(processes->pipes[j].out) == -1)
			fprintf(LoggingFile, UsedPipeWriteErrorCloseFmtLog, (unsigned)time(NULL), j, processes->pipes[j].out, id, strerror(errno));
		else
			fprintf(LoggingFile, UsedPipeWriteCloseFmtLog, (unsigned)time(NULL), j, processes->pipes[j].out, id);
	}
}

void closeUsedPipes(int id, Process *processes, FILE *LoggingFile)
{
    for (int j = 0; j < processes->count; j++)
	{
		closeUsedPipesIn(LoggingFile, PARENT_ID, j, processes);
		closeUsedPipesOut(LoggingFile, PARENT_ID, j, processes);
    }

    free(processes->pipes);
}

void doForks(Process *processes, int numberOfProcesses, FILE *LoggingFile, FILE *EventsLoggingFile)
{
	pid_t pid;
	for (int i = 1; i < numberOfProcesses; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			closeUnusedPipes(i, numberOfProcesses, LoggingFile, processes);
			child(i, &processes[i], LoggingFile, EventsLoggingFile);
			exit(EXIT_SUCCESS);
		}
	}
}

void doForkWithExtra(Process *processes, int numberOfProcesses, FILE *LoggingFile, FILE *EventsLoggingFile)
{
	doForks(processes, numberOfProcesses, LoggingFile, EventsLoggingFile);
	closeUnusedPipes(PARENT_ID, numberOfProcesses, LoggingFile, processes);
}

void createPipes(Process* processes, FILE *LoggingFile, int numberOfProcesses)
{
	int inFD[2];
	int outFD[2];

	for (int fromProcess = 0; fromProcess < numberOfProcesses; fromProcess++)
	{
		for (int toProcess = fromProcess + 1; toProcess <= numberOfProcesses; toProcess++)
		{
			pipe(inFD);
			pipe(outFD);

			processes[toProcess].pipes[fromProcess].in = outFD[0];
			processes[toProcess].pipes[fromProcess].out = inFD[1];
			processes[fromProcess].pipes[toProcess].in = inFD[0];
			processes[fromProcess].pipes[toProcess].out = outFD[1];

			fprintf(LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), fromProcess, toProcess, outFD[0], outFD[1]);
			fprintf(LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), toProcess, fromProcess, inFD[0], inFD[1]);
		}
	}
}

void recieveAllAndLog(Process* processes, int numberOfProcesses, FILE *EventsLoggingFile)
{
	int rcvDone = 0;

	receiveAll(&processes[0], numberOfProcesses - 1, STARTED, &rcvDone);
	eventLog(EventsLoggingFile, log_received_all_started_fmt, PARENT_ID);

	receiveAll(&processes[0], numberOfProcesses - 1 - rcvDone, DONE, &rcvDone);
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

	createPipes(processes, LoggingFile, numberOfProcesses);

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
