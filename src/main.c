#include "mbed-common.h"
#include "hw.h"
#include "time.h"
#include "alloc.h"
#include "logging.h"
#include "board.h"

DEFINE_ALLOC_MEMPOOL(pool32, 64, 32);

static void mempool_init(void)
{
  alloc_init();
  alloc_mempool_add(&pool32);
}

int main(void)
{
  int cnt = 1;
  timer tmp_timer = STOPPED_TIMER;
  timer_start(&tmp_timer, 1000);

  board_preinit();
  board_init();
  mempool_init();
  log_init();
  log("CDC ACM starting\n");

  while (1) {
    board_poll();
    log_poll();
  }

  return 0;
}