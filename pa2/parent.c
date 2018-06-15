#define _GNU_SOURCE
#include "header.h"
#include <fcntl.h>
#include <sys/wait.h>

void saveHistory(void *buf, Message *msg, int rcv) {
    BalanceHistory *history = (BalanceHistory *) buf;
  memcpy(&history[rcv], (BalanceHistory *)msg->s_payload, sizeof(BalanceHistory));
}

void realCloseUnusedPipes(ArrayOfPipes *processesPipes, Process *process, int k, int j)
{
	if (processesPipes[k].pipes[j].out != -1)
	{
		if (close(processesPipes[k].pipes[j].out) == -1)
			fprintf(process->LoggingFile, UnusedPipeWriteErrorCloseFmtLog, (unsigned)time(NULL), k, processesPipes[k].pipes[j].out, k, strerror(errno));
		else
			fprintf(process->LoggingFile, UnusedPipeWriteCloseFmtLog, (unsigned)time(NULL), k, processesPipes[k].pipes[j].out, k);
	}
	if (processesPipes[k].pipes[j].in != -1)
	{
		if (close(processesPipes[k].pipes[j].in) == -1)
			fprintf(process->LoggingFile, UnusedPipeReadErrorCloseFmtLog, (unsigned)time(NULL), k, processesPipes[k].pipes[j].in, j, strerror(errno));
		else
			fprintf(process->LoggingFile, UnusedPipeReadCloseFmtLog, (unsigned)time(NULL), k, processesPipes[k].pipes[j].in, j);
	}
}

void freeUnusedPipesMemory(ArrayOfPipes *processesPipes, Process *process)
{
	for (int i = 1; i < process->arrayOfPipes->count; i++)
	{
		if (i == process->id) continue;
		else free(processesPipes[i].pipes);
	}
}

void closeUnusedPipes(ArrayOfPipes *processesPipes, Process *process)
{
	for (int i = 0; i < process->arrayOfPipes->count; i++)
	{
		if (i == process->id) continue;
		else
		{
			for (int j = 0; j < processesPipes[i].count; j++)
			{
				realCloseUnusedPipes(processesPipes, process, i, j);
			}
		}
	}

	freeUnusedPipesMemory(processesPipes, process);
}

void closeUsedPipesIn(int j, Process *childProcess)
{
	if (childProcess->arrayOfPipes->pipes[j].in != -1)
	{
		if (close(childProcess->arrayOfPipes->pipes[j].in) == -1)
			fprintf(childProcess->LoggingFile, UsedPipeReadErrorCloseFmtLog, 
			(unsigned)time(NULL), j, childProcess->arrayOfPipes->pipes[j].in, childProcess->id, strerror(errno));
		else
			fprintf(childProcess->LoggingFile, UsedPipeReadCloseFmtLog, 
			(unsigned)time(NULL), j, childProcess->arrayOfPipes->pipes[j].in, childProcess->id);
	}
}

void closeUsedPipesOut(int j, Process *childProcess)
{
	if (childProcess->arrayOfPipes->pipes[j].out != -1)
	{
		if (close(childProcess->arrayOfPipes->pipes[j].out) == -1)
			fprintf(childProcess->LoggingFile, UsedPipeWriteErrorCloseFmtLog,
			(unsigned)time(NULL), j, childProcess->arrayOfPipes->pipes[j].out, childProcess->id, strerror(errno));
		else
			fprintf(childProcess->LoggingFile, UsedPipeWriteCloseFmtLog,
			(unsigned)time(NULL), j, childProcess->arrayOfPipes->pipes[j].out, childProcess->id);
	}
}

void closeUsedPipes(Process *childProcess)
{
	for (int j = 0; j < childProcess->arrayOfPipes->count; j++)
	{
		closeUsedPipesIn(j, childProcess);
		closeUsedPipesOut(j, childProcess);
	}

	free(childProcess->arrayOfPipes->pipes);
}

void createPipes(ArrayOfPipes *processesPipes, Process *parentProcess, int numberOfProcesses)
{
	int inFD[2];
	int outFD[2];

	for (int fromProcess = 0; fromProcess < numberOfProcesses; fromProcess++)
	{
		for (int toProcess = fromProcess + 1; toProcess < numberOfProcesses; toProcess++)
		{
			pipe2(inFD, O_NONBLOCK);
			pipe2(outFD, O_NONBLOCK);

			processesPipes[toProcess].pipes[fromProcess].in = outFD[0];
			processesPipes[toProcess].pipes[fromProcess].out = inFD[1];
			processesPipes[fromProcess].pipes[toProcess].in = inFD[0];
			processesPipes[fromProcess].pipes[toProcess].out = outFD[1];

			fprintf(parentProcess->LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), fromProcess, toProcess, outFD[0], outFD[1]);
			fprintf(parentProcess->LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), toProcess, fromProcess, inFD[0], inFD[1]);
		}
	}
}

void doForks(ArrayOfPipes *processesPipes, Process *parentProcess, int *startingBalances)
{
	pid_t pid;
	for (int i = 1; i < parentProcess->arrayOfPipes->count; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			Process childProcess = {
				.id = i,
				.EventsLoggingFile = parentProcess->EventsLoggingFile,
				.LoggingFile = parentProcess->LoggingFile,
				.currentBalance = startingBalances[i - 1],
				.arrayOfPipes = &processesPipes[i],
			};

			closeUnusedPipes(processesPipes, &childProcess);
			child(&childProcess);
			fclose(childProcess.LoggingFile);
			fclose(childProcess.EventsLoggingFile);
			closeUsedPipes(&childProcess);
			exit(EXIT_SUCCESS);
		}
	}
}

void doForkWithExtra(ArrayOfPipes *processesPipes, Process *parentProcess, int *startingBalances)
{
	doForks(processesPipes, parentProcess, startingBalances);
	closeUnusedPipes(processesPipes, parentProcess);
}

void recieveAllAndLog(Process* parentProcess, local_id numberOfChildren)
{
	receiveAll(parentProcess, numberOfChildren, STARTED, NULL, NULL);
	saveToLog(parentProcess->EventsLoggingFile, log_received_all_started_fmt, get_physical_time(), parentProcess->id);
	bank_robbery(parentProcess, numberOfChildren);
	Message *message = createMessage(NULL, 0, STOP, get_physical_time());
	send_multicast(parentProcess, message);
	free((char *)message);
	receiveAll(parentProcess, numberOfChildren, DONE, NULL, NULL);
	saveToLog(parentProcess->EventsLoggingFile, log_received_all_done_fmt, get_physical_time(), parentProcess->id);
}

void printHistory(Process *pInfo, AllHistory *aHistory, int chldCount) {

  const AllHistory *cHistory;
  cHistory = aHistory;
  for (int i = 0; i < cHistory->s_history_len; i++) {
    printf("len %d\n", cHistory->s_history[i].s_history_len);
    fflush(stdout);
  }
  int maxLen = 0;

  for (int i = 0; i < chldCount; i++) {
    if (aHistory->s_history[i].s_history_len > maxLen)
      maxLen = aHistory->s_history[i].s_history_len;
  }
  printf("5 -%d  %d\n", maxLen, cHistory->s_history[4].s_history[6].s_balance);
  printf("5 -%d  %d\n", maxLen, cHistory->s_history[4].s_history[5].s_balance);

  int len;
  for (int i = 0; i < chldCount; i++) {
    len = aHistory->s_history[i].s_history_len;
    if (len < maxLen) {
      BalanceState bs = aHistory->s_history[i].s_history[len - 1];
      for (int j = len; j < maxLen; j++) {
        bs.s_time = j;
        aHistory->s_history[i].s_history[j] = bs;
      }
      aHistory->s_history[i].s_history_len = maxLen;
    }
  }

  print_history(cHistory);
}

int main(int argc, char *argv[]) 
{
	local_id numberOfChildren = atoi(argv[2]);
	int numberOfProcesses = numberOfChildren + 1;
	const int firstBalanceIndex = 3;
	int numberOfBalances = argc - firstBalanceIndex;
	int startingBalances[numberOfBalances];
	for (int i = 0; i < numberOfBalances; i++)
		startingBalances[i] = atoi(argv[firstBalanceIndex + i]);

	Process parentProcess = 
	{
		.id = PARENT_ID,
		.EventsLoggingFile = fopen(events_log, "a"),
		.LoggingFile = fopen(pipes_log, "a"),
	};

	int numberOfPipes = 0;
	for (int i = 1; i < numberOfProcesses; i++)
		numberOfPipes += i;

	ArrayOfPipes *processesPipes = malloc(numberOfProcesses * sizeof(ArrayOfPipes));

	for (int i = 0; i < numberOfProcesses; i++)
	{
		processesPipes[i].count = numberOfProcesses;
		processesPipes[i].pipes = malloc(numberOfProcesses * sizeof(PipeDescriptor));
	}

	createPipes(processesPipes, &parentProcess, numberOfProcesses);
	
	for (int i = 0; i < numberOfProcesses; i++)
	{
		processesPipes[i].pipes[i].out = -1;
		processesPipes[i].pipes[i].in = -1;
	}

	parentProcess.arrayOfPipes = &processesPipes[0];

	doForkWithExtra(processesPipes, &parentProcess, startingBalances);

	recieveAllAndLog(&parentProcess, numberOfChildren);

  AllHistory *aHistory = malloc(sizeof(AllHistory));
  aHistory->s_history_len = numberOfChildren;
  if (receiveAll(&parentProcess, numberOfChildren, BALANCE_HISTORY,
                        aHistory->s_history, saveHistory) == -1)
    error("receiveAll  BALANCE_HISTORY error");

  printHistory(&parentProcess, aHistory, numberOfChildren);

  int finished = 0;
  pid_t wpid;
  while (finished < numberOfProcesses && (wpid = wait(NULL)) > 0) {
    finished++;
  }

  free(aHistory);
  closeUsedPipes(&parentProcess);

  fclose(parentProcess.LoggingFile);
  fclose(parentProcess.EventsLoggingFile);

  exit(EXIT_SUCCESS);
}
