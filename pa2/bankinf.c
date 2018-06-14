#include "header.h"

void transfer (void *parent_data, local_id src, local_id dst, balance_t amount) {
  TransferOrder tOrder = {
    .s_src = src,
    .s_dst = dst,
    .s_amount = amount
  };

  const Message *msg = createMessage((char *) &tOrder,
                              sizeof (TransferOrder),
                              TRANSFER,
                              get_physical_time()
                              );
  send(parent_data, src, msg);
  free((char *)msg);
  Message received;
  receive(parent_data, dst, &received);
  if (received.s_header.s_magic != MESSAGE_MAGIC && received.s_header.s_type != ACK){
      fprintf(stderr, "tranfer error\n");
  }

}
