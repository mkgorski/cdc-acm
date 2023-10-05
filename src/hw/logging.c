#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencmsis/core_cm3.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "stringstream.h"
#include "time.h"
#include "logging.h"

#define BUF_SIZE 1024

static void log_hw_init(void);
static void log_hw_deinit(void);
static void log_hw_send(uint8_t *buf, uint32_t len);
static uint8_t *next_buffer(void);

static int tx_ready = 1;
static stringstream log_stream;

void log_init(void)
{
  log_hw_init();
  ss_init(&log_stream, next_buffer(), BUF_SIZE);
}

void log_deinit(void)
{
  log_hw_deinit();
}

void log_poll(void)
{
  __disable_irq();
  if (tx_ready && ostring_bytes_left(&log_stream) < BUF_SIZE) {
    log_hw_send(log_stream.s, log_stream.p);
    ss_init(&log_stream, next_buffer(), BUF_SIZE);
  }
  __enable_irq();
}

void log(char *fmt, ...)
{   
  char formatted_string[256];

  __disable_irq();
  int len = sprintf(formatted_string, "[%lu.%03lu] ", current_time() / 1000, current_time() % 1000);
  if (len <= 0) goto __log_end;

  va_list argptr;
  va_start(argptr, fmt);
  len += vsprintf(&formatted_string[len], fmt, argptr);
  va_end(argptr);

  if (len > 0 && len < (int)sizeof(formatted_string)) {
    ostring_put(&log_stream, (uint8_t *)formatted_string, len);
  }
__log_end:
  __enable_irq();
}

void log_hex(char *msg, void *buf, uint32_t size)
{
  static char hex[16] = "0123456789abcdef";
  char formatted_string[256];
  uint8_t *buffer = (uint8_t *)buf;

  __disable_irq();
  int len = sprintf(formatted_string, "[%lu.%03lu] ", current_time() / 1000, current_time() % 1000);
  if (len > 0 && len < (int)sizeof(formatted_string)) {
    ostring_put(&log_stream, (uint8_t *)formatted_string, len);
  }
  else {
    goto __log_hex_end;
  }

  ostring_put(&log_stream, (uint8_t *)msg, strlen(msg));
  ostring_putc(&log_stream, ' ');
  for(uint32_t i=0; i<size; i++) {
    ostring_putc(&log_stream, hex[(*buffer >> 4) & 0x0f]);
    ostring_putc(&log_stream, hex[*buffer & 0x0f]);
    ostring_putc(&log_stream, ' ');
    buffer++;
  }
  ostring_putc(&log_stream, '\n');
__log_hex_end:
  __enable_irq();
}

static void log_hw_init(void)
{
  /* clock & pin */
  rcc_periph_clock_enable(RCC_USART1);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6); // TxD
  gpio_set_af(GPIOB, GPIO_AF7, GPIO6);

  /* usart registers */
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  usart_enable(USART1);

  /* dma tx */
  rcc_periph_clock_enable(RCC_DMA2);
  nvic_enable_irq(NVIC_DMA2_STREAM7_IRQ);

  dma_stream_reset(DMA2, DMA_STREAM7);
  dma_set_priority(DMA2, DMA_STREAM7, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA2, DMA_STREAM7, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM7, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM7);
  dma_set_transfer_mode(DMA2, DMA_STREAM7, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA2, DMA_STREAM7, (uint32_t)&USART1_DR);
  dma_channel_select(DMA2, DMA_STREAM7, DMA_SxCR_CHSEL_4);

  /* flush timer */
  rcc_periph_clock_enable(RCC_TIM5);
  rcc_periph_reset_pulse(RST_TIM5);
  timer_set_prescaler(TIM5, 8399); /* 84Mhz/10000hz - 1*/
  timer_set_period(TIM5, 1000);

  nvic_set_priority(NVIC_TIM5_IRQ, 0xff);
  nvic_enable_irq(NVIC_TIM5_IRQ);
  timer_enable_irq(TIM5, TIM_DIER_UIE);
  timer_enable_counter(TIM5);
}

static void log_hw_deinit(void)
{
  // nvic_disable_irq(NVIC_TIM7_IRQ);
  // timer_disable_irq(TIM7, TIM_DIER_UIE);
  // timer_disable_counter(TIM7);

  // nvic_disable_irq(NVIC_DMA2_STREAM7_IRQ);
  // dma_stream_reset(DMA2, DMA_STREAM7);
  // dma_disable_transfer_complete_interrupt(DMA2, DMA_STREAM7);
  // dma_disable_stream(DMA2, DMA_STREAM7);
  // usart_disable_tx_dma(USART1);
  // usart_disable(USART1);

  // nvic_clear_pending_irq(NVIC_DMA2_STREAM7_IRQ);
  // rcc_periph_clock_disable(RCC_DMA2);
  // nvic_clear_pending_irq(NVIC_TIM7_IRQ);
  // rcc_periph_clock_disable(RCC_TIM7);
}

static void log_hw_send(uint8_t *buf, uint32_t len)
{
  tx_ready = 0;
  dma_set_memory_address(DMA2, DMA_STREAM7, (uint32_t)buf);
  dma_set_number_of_data(DMA2, DMA_STREAM7, len);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM7);
  dma_enable_stream(DMA2, DMA_STREAM7);
  usart_enable_tx_dma(USART1);
}

void dma2_stream7_isr(void)
{
  if (dma_get_interrupt_flag(DMA2, DMA_STREAM7, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM7, DMA_TCIF);
  }

  dma_disable_transfer_complete_interrupt(DMA2, DMA_STREAM7);
  usart_disable_tx_dma(USART1);
  tx_ready = 1;
}

static uint8_t *next_buffer(void)
{
  static uint8_t buffers[2][BUF_SIZE];
  static int ptr = 0;

  if (++ptr > 1) ptr = 0;
  return buffers[ptr];
}

void tim5_isr(void)
{
  TIM_SR(TIM5) &= ~TIM_SR_UIF;
  log_poll();
}