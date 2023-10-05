#ifndef _TIME_H_
#define _TIME_H_

void time_init (int divider);
void time_deinit (void);
void sleep (uint32_t interval);
uint32_t current_time (void);

#endif /* _TIME_H_ */