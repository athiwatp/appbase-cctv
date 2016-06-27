/*
 * frame.h
 *
 *  Created on: 23 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef FRAME_H_
#define FRAME_H_

struct frame {
	size_t frame_size;
	size_t frame_bytes_used;
	struct timeval capture_time;
	unsigned char *frame_data;
	size_t width;
	size_t height;
	int format;
};

#endif /* FRAME_H_ */
