#include "mbed-common.h"
#include "hw.h"
#include "time.h"
#include "alloc.h"
#include "logging.h"
#include "board.h"
#include "usb.h"

int main(void)
{
  board_preinit();
  board_init();
  log_init();
  usb_init();
  log("CDC ACM starting\n");

	while (1) {
		usb_poll();
    board_poll();
    log_poll();
	}
}