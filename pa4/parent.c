#define _GNU_SOURCE

#include <fcntl.h>
#include <sys/wait.h>

#include "header.h"

void error(const char *msg) 
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void closeUnsuedPipes(ProcessInfo *pInfo, Process *process) 
{
	for (int i = 0; i < pInfo->process->count; i++) 
	{ 
		if (i != pInfo->id) 
		{
			for (int j = 0; j < process[i].count; j++) 
			{
				if (process[i].pipes[j].in != -1) 
				{
					if (close(process[i].pipes[j].in) == -1)
					fprintf(pInfo->fPipesLog, LogUnusedPipeReadErrorCloseFmt,
							(unsigned)time(NULL), i, process[i].pipes[j].in, j,
							strerror(errno));
					else
					fprintf(pInfo->fPipesLog, LogUnusedPipeReadCloseFmt,
							(unsigned)time(NULL), i, process[i].pipes[j].in, j);
				}

				if (process[i].pipes[j].out != -1) 
				{
					if (close(process[i].pipes[j].out) == -1)
					fprintf(pInfo->fPipesLog, LogUnusedPipeWriteErrorCloseFmt,
							(unsigned)time(NULL), i, process[i].pipes[j].out, i,
							strerror(errno));
					else
					fprintf(pInfo->fPipesLog, LogUnusedPipeWriteCloseFmt,
							(unsigned)time(NULL), i, process[i].pipes[j].out, i);
				}
			}
		}
	}

	for (int i = 1; i < pInfo->process->count; i++) 
	{
		if (i != pInfo->id)
			free(process[i].pipes);
	}
}

void closeUsedPipes(ProcessInfo *pInfo) 
{
	for (int j = 0; j < pInfo->process->count; j++) 
	{
		if (pInfo->process->pipes[j].in != -1) 
		{
			if (close(pInfo->process->pipes[j].in) == -1)
			fprintf(pInfo->fPipesLog, LogUsedPipeReadErrorCloseFmt,
					(unsigned)time(NULL), j, pInfo->process->pipes[j].in,
					pInfo->id, strerror(errno));
			else
			fprintf(pInfo->fPipesLog, LogUsedPipeReadCloseFmt, (unsigned)time(NULL),
					j, pInfo->process->pipes[j].in, pInfo->id);
		}

		if (pInfo->process->pipes[j].out != -1) 
		{
			if (close(pInfo->process->pipes[j].out) == -1)
			fprintf(pInfo->fPipesLog, LogUsedPipeWriteErrorCloseFmt,
					(unsigned)time(NULL), j, pInfo->process->pipes[j].out,
					pInfo->id, strerror(errno));
			else
			fprintf(pInfo->fPipesLog, LogUsedPipeWriteCloseFmt,
					(unsigned)time(NULL), j, pInfo->process->pipes[j].out,
					pInfo->id);
		}
	}

	free(pInfo->process->pipes);
}

Process *createPipes(ProcessInfo *pInfo, int procCount) 
{
	int pipesCount = 0;
	for (int i = procCount - 1; i > 0; i--)
		pipesCount += i;

	Process *procPipes = malloc(procCount * sizeof(Process));
	for (int i = 0; i < procCount; i++) 
	{
		procPipes[i].pipes = malloc(procCount * sizeof(PipeDescriptor));
		procPipes[i].count = procCount;
	}

	int inFD[2], outFD[2];
	for (int fromProcess = 0; fromProcess < procCount; fromProcess++)
	{
		for (int toProcess = fromProcess + 1; toProcess < procCount; toProcess++)
		{
			if (pipe2(inFD, O_NONBLOCK) < 0 || pipe2(outFD, O_NONBLOCK) < 0)
				error("child pipe call error");

			procPipes[fromProcess].pipes[toProcess].in = inFD[0];
			procPipes[fromProcess].pipes[toProcess].out = outFD[1];
			procPipes[toProcess].pipes[fromProcess].in = outFD[0];
			procPipes[toProcess].pipes[fromProcess].out = inFD[1];

			fprintf(pInfo->fPipesLog, LogPipeOpenFmt, (unsigned)time(NULL), fromProcess, toProcess,
				outFD[0], outFD[1]);
			fprintf(pInfo->fPipesLog, LogPipeOpenFmt, (unsigned)time(NULL), toProcess, fromProcess,
				inFD[0], inFD[1]);
		}
	}

	for (int i = 0; i < procCount; i++) 
	{
		procPipes[i].pipes[i].in = -1;
		procPipes[i].pipes[i].out = -1;
	}

	return procPipes;
}

void letsFork(ProcessInfo *pInfo, Process *pl, int childCount, bool isMutex)
{
	pid_t pid;
	for (local_id i = 1; i < pInfo->process->count; i++) 
	{
		pid = fork();
		if (pid == 0) 
		{
			ProcessInfo childPorc = {
				.isMutex = isMutex,
				.workers = childCount,
				.id = i,
				.process = &pl[i],
				.curBalance = 0,
				.fEventsLog = pInfo->fEventsLog,
				.fPipesLog = pInfo->fPipesLog,
			};

			closeUnsuedPipes(&childPorc, pl);
			if (child(&childPorc) == -1) 
			{
				error("child");
			} else 
			{
				fclose(childPorc.fEventsLog);
				fclose(childPorc.fPipesLog);
				closeUsedPipes(&childPorc);
				exit(EXIT_SUCCESS);
			}
		} else 
			if (pid == -1)
				error("fork error");
	}
}

int parseArgs(int argc, char *argv[], bool *isMutex, local_id *childCount) 
{
	int k = 1;
	while (k < argc) 
	{
		if (argv[k][0] == '-') 
		{
			switch (argv[k][1]) 
			{
				case 'p':
					k++;
					*childCount = atoi(argv[k]);
					k++;
					if (*childCount <= 0) 
					{
						fprintf(stderr, "%s error: %d is an invalid number of processes\n",
								argv[0], *childCount);
						return -1;
					}
					break;
				case '-':
					if (strcmp("--mutexl", argv[k]) == 0)
						*isMutex = true;
					k++;
					break;
				default:
					fprintf(stderr, "Usage: %s -p processes_count\n", argv[0]);
					return -1;
			}
		} else {
			fprintf(stderr, "Usage:\t%s -p processes_count\n\t%s -p process_count --mutex", argv[0], argv[0]);
			return -1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) 
{
	int procCount = 0;
	local_id chldCount = 0;

	if (argc < 3) 
	{
		fprintf(stderr, "%s error: not enough arguments\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	bool isMutex = false;

	if (parseArgs(argc, argv, &isMutex, &chldCount) == -1)
	exit(EXIT_FAILURE);

	procCount = chldCount + 1;

	ProcessInfo parentProc = {
		.isMutex = isMutex,
		.workers = chldCount,
		.id = PARENT_ID,
		.fEventsLog = fopen(events_log, "a"),
		.fPipesLog = fopen(pipes_log, "a"),
	};

	Process *procPipes = createPipes(&parentProc, procCount);
	parentProc.process = &procPipes[0];

	letsFork(&parentProc, procPipes,chldCount, isMutex);

	closeUnsuedPipes(&parentProc, procPipes);

	if (receiveAll(&parentProc, chldCount, STARTED, NULL, NULL) == -1)
		error("receiveAll error");
	writeLog (parentProc.fEventsLog, log_received_all_started_fmt,
	get_lamport_time(), parentProc.id);


	if (receiveAll(&parentProc, chldCount, DONE, NULL, NULL) == -1)
		error("receiveAll DONE error");
	writeLog (parentProc.fEventsLog, log_received_all_done_fmt,
	get_lamport_time(), parentProc.id);

	int finished = 0;
	pid_t wpid;
	while (finished < chldCount && (wpid = wait(NULL)) > 0) 
	{
		finished++;
	}

	closeUsedPipes(&parentProc);

	fclose(parentProc.fPipesLog);
	fclose(parentProc.fEventsLog);

	exit(EXIT_SUCCESS);
}
