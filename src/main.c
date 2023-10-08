#include "mbed-common.h"
#include "hw.h"
#include "time.h"
#include "alloc.h"
#include "logging.h"
#include "board.h"
#include "cdc_bridge.h"

int main(void)
{
  board_preinit();
  board_init();
  log_init();
  cdc_brigde_init();
  log("CDC ACM starting\n");

	while (1) {
		cdc_bridge_poll();
    board_poll();
    log_poll();
	}
}