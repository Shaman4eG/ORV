#define _GNU_SOURCE
#include "header.h"
#include <fcntl.h>
#include <sys/wait.h>

void error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void saveHistory(void *buf, Message *msg, int rcv) {
    BalanceHistory *history = (BalanceHistory *) buf;
  memcpy(&history[rcv], (BalanceHistory *)msg->s_payload, sizeof(BalanceHistory));
}

void closeUnsuedPipes(ProcessInfo *pInfo, PipesList *cp) {
  for (int i = 0; i < pInfo->pipesList->count; i++) { // TODO another function
    if (i != pInfo->id) {
      for (int j = 0; j < cp[i].count; j++) {
        if (cp[i].pipes[j].in != -1) {
          if (close(cp[i].pipes[j].in) == -1)
            fprintf(pInfo->fPipesLog, LogUnusedPipeReadErrorCloseFmt,
                    (unsigned)time(NULL), i, cp[i].pipes[j].in, j,
                    strerror(errno));
          else
            fprintf(pInfo->fPipesLog, LogUnusedPipeReadCloseFmt,
                    (unsigned)time(NULL), i, cp[i].pipes[j].in, j);
        }

        if (cp[i].pipes[j].out != -1) {
          if (close(cp[i].pipes[j].out) == -1)
            fprintf(pInfo->fPipesLog, LogUnusedPipeWriteErrorCloseFmt,
                    (unsigned)time(NULL), i, cp[i].pipes[j].out, i,
                    strerror(errno));
          else
            fprintf(pInfo->fPipesLog, LogUnusedPipeWriteCloseFmt,
                    (unsigned)time(NULL), i, cp[i].pipes[j].out, i);
        }
      }
    }
  }

  for (int i = 1; i < pInfo->pipesList->count; i++) {
    if (i != pInfo->id)
      free(cp[i].pipes);
  }
}

void closeUsedPipes(ProcessInfo *pInfo) {
  for (int j = 0; j < pInfo->pipesList->count; j++) {
    if (pInfo->pipesList->pipes[j].in != -1) {
      if (close(pInfo->pipesList->pipes[j].in) == -1)
        fprintf(pInfo->fPipesLog, LogUsedPipeReadErrorCloseFmt,
                (unsigned)time(NULL), j, pInfo->pipesList->pipes[j].in,
                pInfo->id, strerror(errno));
      else
        fprintf(pInfo->fPipesLog, LogUsedPipeReadCloseFmt, (unsigned)time(NULL),
                j, pInfo->pipesList->pipes[j].in, pInfo->id);
    }

    if (pInfo->pipesList->pipes[j].out != -1) {
      if (close(pInfo->pipesList->pipes[j].out) == -1)
        fprintf(pInfo->fPipesLog, LogUsedPipeWriteErrorCloseFmt,
                (unsigned)time(NULL), j, pInfo->pipesList->pipes[j].out,
                pInfo->id, strerror(errno));
      else
        fprintf(pInfo->fPipesLog, LogUsedPipeWriteCloseFmt,
                (unsigned)time(NULL), j, pInfo->pipesList->pipes[j].out,
                pInfo->id);
    }
  }

  free(pInfo->pipesList->pipes);
}

PipesList *createPipes(ProcessInfo *pInfo, int procCount) {

  int pipesCount = 0;
  for (int i = procCount - 1; i > 0; i--)
    pipesCount += i;

  PipesList *procPipes = malloc(procCount * sizeof(PipesList));
  for (int i = 0; i < procCount; i++) {
    procPipes[i].pipes = malloc(procCount * sizeof(PipeDesc));
    procPipes[i].count = procCount;
  }

  int n = 0;
  int k = n + 1;
  int inFD[2], outFD[2];
  for (int i = 0; i < pipesCount; i++) {

    if (pipe2(inFD, O_NONBLOCK) < 0 || pipe2(outFD, O_NONBLOCK) < 0)
      error("child pipe call error");

    // 0 - read
    // 1 - write
    procPipes[n].pipes[k].in = inFD[0];
    procPipes[n].pipes[k].out = outFD[1];
    procPipes[k].pipes[n].in = outFD[0];
    procPipes[k].pipes[n].out = inFD[1];

    fprintf(pInfo->fPipesLog, LogPipeOpenFmt, (unsigned)time(NULL), n, k,
            outFD[0], outFD[1]);
    fprintf(pInfo->fPipesLog, LogPipeOpenFmt, (unsigned)time(NULL), k, n,
            inFD[0], inFD[1]);

    k++;

    if (k >= procCount) {
      n++;
      k = n + 1;
    }
  }

  for (int i = 0; i < procCount; i++) {
    procPipes[i].pipes[i].in = -1;
    procPipes[i].pipes[i].out = -1;
  }

  return procPipes;
}

void letsFork(ProcessInfo *pInfo, int *initBalances, PipesList *pl) {

  pid_t pid;
  for (local_id i = 1; i < pInfo->pipesList->count; i++) {
    // printf("loop iteration %d\n", i);
    pid = fork();
    if (pid == 0) {
      ProcessInfo childPorc = {
          .id = i,
          .pipesList = &pl[i],
          .curBalance = initBalances[i - 1],
          .fEventsLog = pInfo->fEventsLog,
          .fPipesLog = pInfo->fPipesLog,
      };

      closeUnsuedPipes(&childPorc, pl);
      if (child(&childPorc) == -1) {
        error("child");
      } else {
        fclose(childPorc.fEventsLog);
        fclose(childPorc.fPipesLog);
        closeUsedPipes(&childPorc);
        exit(EXIT_SUCCESS);
      }
    } else if (pid == -1)
      error("fork error");
  }
}

void printHistory(ProcessInfo *pInfo, AllHistory *aHistory, int chldCount) {

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

int main(int argc, char *argv[]) {

  int procCount = 0;
  local_id chldCount = 0;

  if (argc < 5) {
    fprintf(stderr, "%s error: not enough arguments\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (argv[1][0] == '-') {
    switch (argv[1][1]) {
    case 'p':
      chldCount = atoi(argv[2]);
      if (chldCount <= 0) {
        fprintf(stderr, "%s error: %d is an invalid number of processes\n",
                argv[0], chldCount);
        exit(EXIT_FAILURE);
      }
      procCount = chldCount + 1;
      break;
    default:
      fprintf(stderr, "Usage: %s -p processes_count\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  int blncCnt = argc - 3;
  if (blncCnt != chldCount) {
    fprintf(stderr,
            "%s error: not enough inicial initBalances. %d instead of %d\n",
            argv[0], blncCnt, chldCount);
    exit(EXIT_FAILURE);
  }

  int initBalances[blncCnt];

  for (int i = 0; i < blncCnt; i++)
    initBalances[i] = atoi(argv[3 + i]);

  ProcessInfo parentProc = {
      .id = PARENT_ID,
      .fEventsLog = fopen(events_log, "a"),
      .fPipesLog = fopen(pipes_log, "a"),
  };

  PipesList *procPipes = createPipes(&parentProc, procCount);
  parentProc.pipesList = &procPipes[0];

  letsFork(&parentProc, initBalances, procPipes);

  closeUnsuedPipes(&parentProc, procPipes);

  // printf("child count is %d\n", chldCount);
  if (receiveAll(&parentProc, chldCount, STARTED, NULL, NULL) == -1)
        error("receiveAll error");
   writeLog (parentProc.fEventsLog, log_received_all_started_fmt,
   get_physical_time(), parentProc.id);

  // printf("before robbery\n");
  // fflush(stdout);
  bank_robbery(&parentProc, chldCount);
//  if (receiveAll(&parentProc, chldCount, ACK, NULL, NULL))
//    error("receiveAll error");

  const Message *msg = createMessage(NULL, 0, STOP, get_physical_time());

  if (send_multicast(&parentProc, msg) == -1) {
    perror("send done error");
    return -1;
  }
  free((char *)msg);

  if (receiveAll(&parentProc, chldCount, DONE, NULL, NULL) == -1)
    error("receiveAll DONE error");
   writeLog (parentProc.fEventsLog, log_received_all_done_fmt,
   get_physical_time(), parentProc.id);

  AllHistory *aHistory = malloc(sizeof(AllHistory));
  aHistory->s_history_len = chldCount;
  if (receiveAll(&parentProc, chldCount, BALANCE_HISTORY,
                        aHistory->s_history, saveHistory) == -1)
    error("receiveAll  BALANCE_HISTORY error");

  printHistory(&parentProc, aHistory, chldCount);

  int finished = 0;
  pid_t wpid;
  while (finished < procCount && (wpid = wait(NULL)) > 0) {
    finished++;
  }

  free(aHistory);
  closeUsedPipes(&parentProc);

  fclose(parentProc.fPipesLog);
  fclose(parentProc.fEventsLog);

  exit(EXIT_SUCCESS);
}
