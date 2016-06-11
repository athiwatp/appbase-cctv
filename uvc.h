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

struct camera_internal;
struct camera {
	char *dev_path;
	size_t frame_size;
	size_t frame_bytes_used;
	char *frame;
	size_t width;
	size_t height;
	int format;
	struct camera_internal *internal;
};

struct camera *uvc_open();

bool uvc_alloc_frame(struct camera *);
bool uvc_capture_frame(struct camera *);
void uvc_close(struct camera *);

#endif /* UVC_H_ */
