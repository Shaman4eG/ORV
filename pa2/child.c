#include <sys/stat.h>
#include "header.h"


void modifyHistory (ProcessInfo *pInfo, balance_t balance, timestamp_t curTime) {
  BalanceHistory *history = pInfo->history;
  timestamp_t lastTime;
  if (curTime != 0)
    lastTime = history->s_history[history->s_history_len - 1].s_time;
  else
    lastTime = 0;
  int diff = curTime - lastTime;
  if (diff > 1) {
    for (timestamp_t i = 1; i < diff &&  history->s_history_len < MAX_T; i++) {
      history->s_history[history->s_history_len] = (BalanceState) {
        .s_balance = history->s_history[history->s_history_len - 1].s_balance,
        .s_time = lastTime + i,
        .s_balance_pending_in = 0
      };
      history->s_history_len++;
    }
  }
  history->s_history[curTime] = (BalanceState) {
    .s_balance = balance,
    .s_time = curTime,
    .s_balance_pending_in = 0
  };
  history->s_history_len = curTime + 1;

  pInfo->curBalance = balance;
}

int transferIn (TransferOrder *tOrder, ProcessInfo *pInfo, timestamp_t time) {
  balance_t curBlnc = pInfo->curBalance + tOrder->s_amount;
  modifyHistory(pInfo, curBlnc, time);
  //printf ("len is %d\n", pInfo->history->s_history_len);

  const Message *msg = createMessage(NULL, 0, ACK, time);
  writeLog(pInfo->fEventsLog, log_transfer_in_fmt, time, pInfo->id, tOrder->s_amount, tOrder->s_src);
  if(send(pInfo, PARENT_ID, msg) == -1) {
    fprintf(stderr, "[child %d] send error: %s\n", pInfo->id, strerror(errno));
    return -1;
  }

  free((char *)msg);

  return 0;
}

int transferOut (Message *msg, ProcessInfo *pInfo, timestamp_t time) {
  TransferOrder *tOrder = (TransferOrder *) msg->s_payload;
  balance_t curBlnc = pInfo->curBalance - tOrder->s_amount;
  modifyHistory(pInfo, curBlnc, time);

  const Message *outMsg = msg;
  writeLog(pInfo->fEventsLog, log_transfer_out_fmt, time, pInfo->id, tOrder->s_amount, tOrder->s_dst);
  if(send(pInfo, tOrder->s_dst, outMsg) == -1) {
    fprintf(stderr, "[child %d] send error: %s\n", pInfo->id, strerror(errno));
    return -1;
  }

  return 0;
}


int sendDone (ProcessInfo *pInfo, timestamp_t time) {
  char messageDone[MAX_PAYLOAD_LEN];
  time = get_physical_time();
  int length = sprintf(messageDone, log_done_fmt, time, pInfo->id, pInfo->curBalance);
  if(length < 0) {
    perror("sprintf done error");
    return -1;
  }
  const Message *msg = createMessage(messageDone, length, DONE, time);

  writeLog(pInfo->fEventsLog, log_done_fmt, time, pInfo->id, pInfo->curBalance);
  if(send_multicast(pInfo, msg) == -1) {
    perror("send done error");
    free((char *)msg);
    return -1;
  }
  free((char *)msg);
  return 0;
}

int sendStart (ProcessInfo *pInfo, timestamp_t time) {
  char messageStarted[MAX_PAYLOAD_LEN];
  time = get_physical_time();
  int length = sprintf(messageStarted, log_started_fmt, time, pInfo->id, pInfo->pid, pInfo->ppid, pInfo->curBalance);
  if (length < 0) {
    perror("sprintf error");
    return -1;
  }
  const Message *msg = createMessage(messageStarted, length, STARTED, time);

  writeLog (pInfo->fEventsLog, log_started_fmt, time, pInfo->id, pInfo->pid, pInfo->ppid, pInfo->curBalance);
  if(send_multicast(pInfo, msg) == -1) {
    fprintf(stderr, "[child %d] send_multicast error: %s", pInfo->id, strerror(errno));
    free((char *)msg);
    return -1;
  }

  free((char *)msg);
  return 0;
}

int sendBalanceHistory (ProcessInfo *pInfo, timestamp_t time) {

  size_t len = sizeof (local_id) + sizeof (uint8_t) + pInfo->history->s_history_len * sizeof (BalanceState);
  //size_t len = sizeof (BalanceHistory);
  char msgBuf[MAX_PAYLOAD_LEN];
  memcpy(msgBuf, pInfo->history, len);

  const Message *msg = createMessage(msgBuf, len, BALANCE_HISTORY, time);

  writeLog (pInfo->fEventsLog, log_received_all_done_fmt, time,  pInfo->id);
  if(send(pInfo, PARENT_ID,  msg) == -1) {
    fprintf(stderr, "[child %d] send to %d error: %s", pInfo->id, PARENT_ID, strerror(errno));
    free((char *)msg);
    return -1;
  }

  free((char *)msg);
  return 0;
}

int child (ProcessInfo *pInfo) {

  pInfo->pid = getpid();
  pInfo->ppid = getppid();

  BalanceHistory *history = malloc(sizeof (BalanceHistory));
  history->s_history_len = 0;
  history->s_id = pInfo->id;
  pInfo->history = history;
  timestamp_t time = get_physical_time();
  modifyHistory(pInfo, pInfo->curBalance, time);

  if (sendStart(pInfo, get_physical_time()) == -1)
    return -1;
  if (receiveAll(pInfo, pInfo->pipesList->count - 1, STARTED, NULL, NULL) == -1)
    return -1;
 writeLog(pInfo->fEventsLog, log_received_all_started_fmt, get_physical_time(), pInfo->id);

  int rcvDone = 0;
  //int rcvStart = 0;
  Message inMsg;
  TransferOrder *tOrder;
  int working = 1;

  do {
    if (receive_any(pInfo, &inMsg) == -1)
      fprintf(stderr, "[child %d] receive_any error: %s\n", pInfo->id, strerror(errno));
    else {
      if (inMsg.s_header.s_magic == MESSAGE_MAGIC) {
        time = get_physical_time();
        switch(inMsg.s_header.s_type) {
          case TRANSFER:
            tOrder = (TransferOrder *) inMsg.s_payload;
            if (tOrder->s_dst == pInfo->id) {
              if (transferIn(tOrder, pInfo, time) == -1) {
                return -1;
              }
            }
            else {
              if (transferOut(&inMsg, pInfo, time) == -1)
                return -1;
            }
            break;
          case STOP:
            if (sendDone(pInfo, time) == -1)
              return -1;
            break;
          case DONE:
            rcvDone++;
            if (rcvDone == pInfo->pipesList->count - 2) {      
              if (sendBalanceHistory(pInfo, time) == -1)
                return -1;
              working = 0;
            }
            break;
          default:
            fprintf(stderr, "[child %d] strange message type (%d)\n", pInfo->id, inMsg.s_header.s_type);
            break;
        }
      }
    }
  } while(working);

  free(history);
  return 0;
}
