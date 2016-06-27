/*
 * appbase.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef APPBASE_H_
#define APPBASE_H_
#include <stdint.h>
#include "main.h"
#include "frame.h"

struct appbase;

struct appbase *appbase_open(const char *app_name,
		const char *username,
		const char *password,
		bool enable_streaming);
bool appbase_push_frame(struct appbase *ab,
		const unsigned char *data,
		uint32_t length,
		struct timeval *timestamp);
bool appbase_fill_frame(struct appbase *, struct frame *);
void appbase_close(struct appbase *);

void appbase_enable_progress(struct appbase *appbase, uint8_t enable);
void appbase_enable_verbose(struct appbase *appbase, uint8_t enable);

#endif /* APPBASE_H_ */
