#define _GNU_SOURCE
#include "header.h"
#include <fcntl.h>
#include <sys/wait.h>

void saveHistory(void *buf, Message *msg, int rcv) {
    BalanceHistory *history = (BalanceHistory *) buf;
  memcpy(&history[rcv], (BalanceHistory *)msg->s_payload, sizeof(BalanceHistory));
}

void closeUnsuedPipes(Process *pInfo, ArrayOfPipes *cp) {
  for (int i = 0; i < pInfo->arrayOfPipes->count; i++) { // TODO another function
    if (i != pInfo->id) {
      for (int j = 0; j < cp[i].count; j++) {
        if (cp[i].pipes[j].in != -1) {
          if (close(cp[i].pipes[j].in) == -1)
            fprintf(pInfo->LoggingFile, UnusedPipeReadErrorCloseFmtLog,
                    (unsigned)time(NULL), i, cp[i].pipes[j].in, j,
                    strerror(errno));
          else
            fprintf(pInfo->LoggingFile, UnusedPipeReadCloseFmtLog,
                    (unsigned)time(NULL), i, cp[i].pipes[j].in, j);
        }

        if (cp[i].pipes[j].out != -1) {
          if (close(cp[i].pipes[j].out) == -1)
            fprintf(pInfo->LoggingFile, UnusedPipeWriteErrorCloseFmtLog,
                    (unsigned)time(NULL), i, cp[i].pipes[j].out, i,
                    strerror(errno));
          else
            fprintf(pInfo->LoggingFile, UnusedPipeWriteCloseFmtLog,
                    (unsigned)time(NULL), i, cp[i].pipes[j].out, i);
        }
      }
    }
  }

  for (int i = 1; i < pInfo->arrayOfPipes->count; i++) {
    if (i != pInfo->id)
      free(cp[i].pipes);
  }
}

void closeUsedPipes(Process *pInfo) {
  for (int j = 0; j < pInfo->arrayOfPipes->count; j++) {
    if (pInfo->arrayOfPipes->pipes[j].in != -1) {
      if (close(pInfo->arrayOfPipes->pipes[j].in) == -1)
        fprintf(pInfo->LoggingFile, UsedPipeReadErrorCloseFmtLog,
                (unsigned)time(NULL), j, pInfo->arrayOfPipes->pipes[j].in,
                pInfo->id, strerror(errno));
      else
        fprintf(pInfo->LoggingFile, UsedPipeReadCloseFmtLog, (unsigned)time(NULL),
                j, pInfo->arrayOfPipes->pipes[j].in, pInfo->id);
    }

    if (pInfo->arrayOfPipes->pipes[j].out != -1) {
      if (close(pInfo->arrayOfPipes->pipes[j].out) == -1)
        fprintf(pInfo->LoggingFile, UsedPipeWriteErrorCloseFmtLog,
                (unsigned)time(NULL), j, pInfo->arrayOfPipes->pipes[j].out,
                pInfo->id, strerror(errno));
      else
        fprintf(pInfo->LoggingFile, UsedPipeWriteCloseFmtLog,
                (unsigned)time(NULL), j, pInfo->arrayOfPipes->pipes[j].out,
                pInfo->id);
    }
  }

  free(pInfo->arrayOfPipes->pipes);
}

void createPipes(ArrayOfPipes *processesPipes, Process *process, int numberOfProcesses)
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

			fprintf(process->LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), fromProcess, toProcess, outFD[0], outFD[1]);
			fprintf(process->LoggingFile, PipeOpenFmtLog, (unsigned)time(NULL), toProcess, fromProcess, inFD[0], inFD[1]);
		}
	}
}

void letsFork(Process *pInfo, int *initBalances, ArrayOfPipes *pl) {

  pid_t pid;
  for (local_id i = 1; i < pInfo->arrayOfPipes->count; i++) {
    // printf("loop iteration %d\n", i);
    pid = fork();
    if (pid == 0) {
      Process childPorc = {
          .id = i,
          .arrayOfPipes = &pl[i],
          .currentBalance = initBalances[i - 1],
          .EventsLoggingFile = pInfo->EventsLoggingFile,
          .LoggingFile = pInfo->LoggingFile,
      };

      closeUnsuedPipes(&childPorc, pl);
      if (child(&childPorc) == -1) {
        error("child");
      } else {
        fclose(childPorc.EventsLoggingFile);
        fclose(childPorc.LoggingFile);
        closeUsedPipes(&childPorc);
        exit(EXIT_SUCCESS);
      }
    } else if (pid == -1)
      error("fork error");
  }
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
		processes[i].pipes[i].out = -1;
		processes[i].pipes[i].in = -1;
	}

	parentProcess.arrayOfPipes = &processesPipes[0];

  letsFork(&parentProcess, startingBalances, processesPipes);

  closeUnsuedPipes(&parentProcess, processesPipes);

  // printf("child count is %d\n", chldCount);
  if (receiveAll(&parentProcess, numberOfChildren, STARTED, NULL, NULL) == -1)
        error("receiveAll error");
   writeLog (parentProcess.EventsLoggingFile, log_received_all_started_fmt,
   get_physical_time(), parentProcess.id);

  // printf("before robbery\n");
  // fflush(stdout);
  bank_robbery(&parentProcess, numberOfChildren);
//  if (receiveAll(&parentProc, chldCount, ACK, NULL, NULL))
//    error("receiveAll error");

  const Message *msg = createMessage(NULL, 0, STOP, get_physical_time());

  if (send_multicast(&parentProcess, msg) == -1) {
    perror("send done error");
    return -1;
  }
  free((char *)msg);

  if (receiveAll(&parentProcess, numberOfChildren, DONE, NULL, NULL) == -1)
    error("receiveAll DONE error");
   writeLog (parentProcess.EventsLoggingFile, log_received_all_done_fmt,
   get_physical_time(), parentProcess.id);

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
