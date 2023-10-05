#include <stdint.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include "time.h"

static volatile uint32_t _current_time;

void time_init (int divider)
{
  systick_set_reload(divider);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();
  systick_interrupt_enable();
}

void time_deinit (void)
{
  systick_counter_disable();
  systick_interrupt_disable();
  SCB_ICSR |= SCB_ICSR_PENDSTCLR;
}

void sleep (uint32_t interval)
{
  uint32_t start = current_time();
  while (current_time() - start < interval);
}

uint32_t current_time(void) {
  return _current_time;
}

void sys_tick_handler(void)
{
  _current_time++;
}
