#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include "logging.h"
#include "uart.h"

typedef struct uart_dma_ctx_t {
  uint8_t *buffer;
  uint32_t offset;
} uart_dma_ctx_t;

static uint8_t rx_buffer[512];
static uart_dma_ctx_t rx_dma;
static uart_event_t tx_event;
static uart_event_t rx_event;
static uart_event_cb_t uart_event_cb = NULL;

static void send_rx_event(uint8_t *buf, uint32_t offset, uint32_t len);

int uart_init(uart_event_cb_t cb)
{
  memset(&tx_event, 0, sizeof(uart_event_t));
  memset(&rx_event, 0, sizeof(uart_event_t));

  /* clock & pin */
  rcc_periph_clock_enable(RCC_USART2);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2); // TxD
  gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3); // RxD
  gpio_set_af(GPIOA, GPIO_AF7, GPIO3);

  /* usart registers */
  usart_set_baudrate(USART2, 1500000);
  usart_set_databits(USART2, 8);
  usart_set_stopbits(USART2, USART_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX_RX);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  rcc_periph_clock_enable(RCC_DMA1);
  /* dma tx */
  nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);
  dma_stream_reset(DMA1, DMA_STREAM6);
  dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_MEDIUM);
  dma_set_memory_size(DMA1, DMA_STREAM6, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM6, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);
  dma_set_transfer_mode(DMA1, DMA_STREAM6, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t)&USART2_DR);
  dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_4);

  /* dma rx */
  nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
  dma_stream_reset(DMA1, DMA_STREAM5);
  dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_MEDIUM);
  dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
  dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_4);
  dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t)&USART2_DR);
  dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t)&rx_buffer[0]);
  dma_set_memory_address_1(DMA1, DMA_STREAM5, (uint32_t)&rx_buffer[sizeof(rx_buffer) / 2]);
  dma_set_number_of_data(DMA1, DMA_STREAM5, sizeof(rx_buffer) / 2);
  dma_enable_double_buffer_mode(DMA1, DMA_STREAM5);

  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
  dma_enable_stream(DMA1, DMA_STREAM5);
  usart_enable_rx_dma(USART2);

  if (cb)
    uart_event_cb = cb;

  rx_dma.buffer = rx_buffer;
  rx_dma.offset = 0;

  nvic_enable_irq(NVIC_USART2_IRQ);
  usart_enable_idle_interrupt(USART2);
  usart_enable(USART2);

  return 0;
}

int uart_send(uint8_t *buf, uint32_t len)
{
  log("-> uart send, rdy=%d len=%d\n", tx_event.data.tx.buf ? 0 : 1, len);
  if (tx_event.data.tx.buf) // tx already in progress
    return -1;

  if (len == 1) {
    usart_wait_send_ready(USART2);
    usart_send(USART2, *buf);
    return 1;
  }

  dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t)buf);
  dma_set_number_of_data(DMA1, DMA_STREAM6, len);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
  dma_enable_stream(DMA1, DMA_STREAM6);
  usart_enable_tx_dma(USART2);
  tx_event.data.tx.buf = buf;
  tx_event.data.tx.len = len;

  return 0;
}

int uart_poll()
{
  return 0;
}

void dma1_stream6_isr(void)
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
  }

  dma_disable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
  usart_disable_tx_dma(USART2);

  if (uart_event_cb) {
    uart_event_cb(&tx_event);
    tx_event.data.tx.buf = NULL;
    tx_event.data.tx.len = 0;
  }
}

void dma1_stream5_isr(void)
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
    uint32_t len = sizeof(rx_buffer) / 2 - rx_dma.offset;
    if (len > 0) {
      // log("data dma callback: buf=%p, offset=%lu, len=%lu\n", rx_dma.buffer, rx_dma.offset, len);
      send_rx_event(rx_dma.buffer, rx_dma.offset, len);

      rx_dma.offset = 0;
      if (dma_get_target(DMA1, DMA_STREAM5))
        rx_dma.buffer += sizeof(rx_buffer) / 2;
      else
        rx_dma.buffer -= sizeof(rx_buffer) / 2;

      gpio_toggle(GPIOB, GPIO13);
    }
  }
  else {
    log("spurious dma interrupt\n");
  }
}

void usart2_isr(void)
{
  if (USART_SR(USART2) & USART_SR_IDLE) {
    uint16_t data = usart_recv(USART2); // clear flag
    uint32_t len = sizeof(rx_buffer) / 2 - rx_dma.offset - dma_get_number_of_data(DMA1, DMA_STREAM5);
    if (len > 0) {
      // log("data uart callback: buf=%p, offset=%lu, len=%lu\n", rx_dma.buffer, rx_dma.offset, len);
      send_rx_event(rx_dma.buffer, rx_dma.offset, len);
      rx_dma.offset += len;
    }
    gpio_toggle(GPIOB, GPIO13);
  }
  else {
    log("spurious uart interrupt\n");
  }
}

static void send_rx_event(uint8_t *buf, uint32_t offset, uint32_t len)
{
  if (uart_event_cb) {
    rx_event.type = UART_RX_RDY;
    rx_event.data.rx.buf = buf;
    rx_event.data.rx.offset = offset;
    rx_event.data.rx.len = len;
    uart_event_cb(&rx_event);
  }
}