#include <string.h>
#include "alloc.h"
#include "dlist.h"
#include "uart.h"
#include "usb.h"
#include "board.h"
#include "cdc_bridge.h"
#include "logging.h"

DEFINE_ALLOC_MEMPOOL(message_pool, 32, 128);

typedef struct {
  dnode_t node;
  int len;
  uint8_t data[];
} message_t;

static dlist_t tx_queue;
static dlist_t rx_queue;

static void usb_rx_cb(uint8_t *buf, uint32_t len);
static void uart_event_cb(uart_event_t *event);
static void btn_poll(void);

int cdc_brigde_init()
{
  alloc_init();
  alloc_mempool_add(&message_pool);
  dlist_init(&tx_queue);
  dlist_init(&rx_queue);

  usb_init(usb_rx_cb);
  uart_init(uart_event_cb);

  return 0;
}


int cdc_bridge_poll()
{
  usb_poll();
  uart_poll();
  btn_poll();

  // USB to UART
  if (!dlist_empty(&tx_queue)) {
    message_t *msg = (message_t *)dlist_peek_head(&tx_queue);
    int ret = uart_send(msg->data, msg->len);

    if (ret == 0) { // send to DMA, deallocate later
      dlist_get_head(&tx_queue);
    }

    if (ret > 0) { // sent immediately, deallocate now
      dlist_get_head(&tx_queue);
      alloc_free(msg);
    }
  }

  // UART to USB
  if (!dlist_empty(&rx_queue)) {
    message_t *msg = (message_t *)dlist_peek_head(&rx_queue);
    int ret = usb_send(msg->data, msg->len);
    log("usb send=%d/%d\n", ret, msg->len);
    if (ret > 0) {
      dlist_get_head(&rx_queue);
      alloc_free(msg);
    }
  }

  return 0;
}

static void usb_rx_cb(uint8_t *buf, uint32_t len)
{
  message_t *msg = alloc_get(sizeof(message_t) + len);
  if (msg) {
    memcpy(msg->data, buf, len);
    msg->len = len;
    dlist_append(&tx_queue, (dnode_t *)msg);
  }
}

static void uart_event_cb(uart_event_t *event)
{
  if (event->type == UART_RX_RDY) {
    int len = event->data.rx.len;
    uint8_t *src = event->data.rx.buf + event->data.rx.offset;
    while (len > 0) {
      int chunk_len = len;
      if (chunk_len > 64)
        chunk_len = 64;

      message_t *msg = alloc_get(sizeof(message_t) + chunk_len);
      if (msg) {
        memcpy(msg->data, src, chunk_len);
        msg->len = chunk_len;
        dlist_append(&rx_queue, (dnode_t *)msg);
      }

      src += chunk_len;
      len -= chunk_len;
    }
  }

  if ((event->type == UART_TX_DONE) || (event->type == UART_TX_ABORTED)) {
    alloc_free(event->data.tx.buf);
  }
}

static void btn_poll()
{
  static int cnt = 0;
  static int pressed = 0;

  if (board_btn_pressed()) {
    if (pressed == 0) {
      cnt++;
      if (cnt > 500) {
        pressed = 1;
        log("btn pressed\n");
      }
    }
  }
  else {
    cnt = 0;
    pressed = 0;
  }
}
