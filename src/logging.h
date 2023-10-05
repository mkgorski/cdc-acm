#ifndef _LOGGING_H_
#define _LOGGING_H_

void log_init(void);
void log_deinit(void);
void log_poll(void);
void log(char *fmt, ...);
void log_hex(char *msg, void *buf, uint32_t len);

#endif /* _LOGGING_H_ */