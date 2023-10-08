#ifndef _BOARD_H_
#define _BOARD_H_

int board_preinit(void);
int board_init(void);
int board_poll(void);
int board_deinit(void);
int board_btn_pressed(void);

#endif /* _BOARD_H_ */
