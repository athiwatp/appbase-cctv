/*
 * frame.h
 *
 *  Created on: 23 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef FRAME_H_
#define FRAME_H_
#include <time.h>

enum frame_format {
	FRAME_FORMAT_FIRST,
	FRAME_FORMAT_YUYV,
	FRAME_FORMAT_YUY2 = FRAME_FORMAT_YUYV,
	FRAME_FORMAT_JPEG,
	FRAME_FORMAT_COUNT
};
#define FRAME_FORMAT_IS_SUPPORTED(f) (f > FRAME_FORMAT_FIRST && f < FRAME_FORMAT_COUNT)

#define DEFAULT_WIDTH	320
#define DEFAULT_HEIGHT	240

struct frame {
	size_t frame_size;
	size_t frame_bytes_used;
	struct timeval capture_time;
	unsigned char *frame_data;
	size_t width;
	size_t height;
	int format;
};

void frame_convert_yuyv_to_jpeg(struct frame *);

#endif /* FRAME_H_ */
