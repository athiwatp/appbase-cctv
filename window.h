/*
 * window.h
 *
 *  Created on: 23 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef WINDOW_H_
#define WINDOW_H_

#include "main.h"
#include "frame.h"

struct window;

struct window *start_window(size_t width, size_t height, int format);
void destroy_window(struct window *);

bool window_render_frame(struct window *, struct frame *);

bool window_is_closed();

#endif /* WINDOW_H_ */
