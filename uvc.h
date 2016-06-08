/*
 * uvc.h
 *
 *  Created on: 3 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef UVC_H_
#define UVC_H_
#include <stdint.h>
#include "utils.h"

struct camera {
	char *dev_path;
	size_t frame_size;
	char *frame;
};

struct camera *uvc_open();

bool uvc_alloc_frame(struct camera *,
		int format,
		size_t width,
		size_t height);
bool uvc_capture_frame(struct camera *);
void uvc_close(struct camera *);

#endif /* UVC_H_ */
