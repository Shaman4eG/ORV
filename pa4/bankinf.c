#include "header.h"
#include <stdbool.h>
#define d 1

static timestamp_t localTime = 0;

void transfer (void *parent_data, local_id src, local_id dst, balance_t amount) {
  TransferOrder tOrder = {
    .s_src = src,
    .s_dst = dst,
    .s_amount = amount
  };

  noteEventForLamportTime();
  const Message *msg = createMessage((char *) &tOrder,
                              sizeof (TransferOrder),
                              TRANSFER,
                              get_lamport_time()
                              );
  send(parent_data, src, msg);
  free((char *)msg);
  Message received;
  receive(parent_data, dst, &received);
  if (received.s_header.s_magic != MESSAGE_MAGIC && received.s_header.s_type != ACK){
      fprintf(stderr, "tranfer error\n");
  }
  synchronizeLamportTime(received.s_header.s_local_time);

}


void synchronizeLamportTime(timestamp_t time){
    if (time > localTime)
        localTime = time;
    localTime++;
}

void noteEventForLamportTime() {
    localTime++;
}

timestamp_t get_lamport_time() {
    return localTime;
}
