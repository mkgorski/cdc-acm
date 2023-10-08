#ifndef _UART_H_
#define _UART_H_

typedef enum {
  UART_TX_DONE,
  UART_TX_ABORTED,
  UART_RX_RDY
} uart_event_type_t;

typedef struct uart_event_tx_t {
	uint8_t *buf;
	uint32_t len;
} uart_event_tx_t;

typedef struct uart_event_rx_t {
	uint8_t *buf;
	uint32_t offset;
	uint32_t len;
} uart_event_rx_t;

typedef struct uart_event_t {
  uart_event_type_t type;
  union uart_event_data {
    uart_event_tx_t tx;
    uart_event_rx_t rx;
  } data;
} uart_event_t;

typedef void (*uart_event_cb_t)(uart_event_t *event);

int uart_init(uart_event_cb_t cb);
int uart_send(uint8_t *buf, uint32_t len);
int uart_poll(void);

#endif /* _UART_H_ */