/*
 * uvc.h
 *
 *  Created on: 3 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef UVC_H_
#define UVC_H_

struct camera;
struct frame {
	size_t length;
	char *data;
};

struct camera *uvc_open();

/* TODO
 * This requires uvc_capture_frame() to allocate memory.
 * Maybe it's not the best way.
 * Are frames of a fixed size?
 */
struct frame *uvc_capture_frame();
void uvc_close(struct camera *c);

#endif /* UVC_H_ */
