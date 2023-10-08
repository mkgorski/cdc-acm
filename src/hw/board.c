#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencmsis/core_cm3.h>
#include "board.h"
#include "time.h"
#include "timer.h"

static timer blink_timer = STOPPED_TIMER;

static void clock_init(void);
static void clock_deinit(void);
static void gpio_init(void);
static void gpio_deinit(void);

int board_preinit(void)
{
  clock_init();
  time_init(SYSTEM_CLOCK / 1000);
  return 0;
}

int board_init(void)
{
  gpio_init();
  timer_start(&blink_timer, 2000);
  return 0;
}

int board_poll(void)
{
  if (timer_expired(&blink_timer)) {
    timer_restart(&blink_timer);
    // gpio_toggle(GPIOC, GPIO13);
    gpio_toggle(GPIOB, GPIO13);
  }
  return 0;
}

int board_deinit(void)
{
  time_deinit();
  clock_deinit();
  gpio_deinit();
  return 0;
}

int board_btn_pressed(void)
{
  return gpio_get(GPIOA, GPIO0) == 0;
}

static void clock_init(void)
{
  rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
}

static void clock_deinit(void)
{
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_init(void)
{
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13); // LED
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12); // DBG1
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13); // DBG2
  gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0); // BTN
}

static void gpio_deinit(void)
{
}
