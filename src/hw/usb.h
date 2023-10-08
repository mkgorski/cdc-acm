#ifndef _USB_H_
#define _USB_H_

typedef void (*usb_recv_cb_t)(uint8_t *buf, uint32_t len);

int usb_init(usb_recv_cb_t cb);
int usb_send(uint8_t *buf, uint32_t len);
int usb_poll(void);

#endif /* _USB_H_ */