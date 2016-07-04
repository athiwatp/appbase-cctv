/*
 * appbase.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef APPBASE_H_
#define APPBASE_H_

#define AB_KEY_IMAGE	"image"
#define AB_KEY_SEC	"sec"
#define AB_KEY_USEC	"usec"

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
		size_t length,
		struct timeval *timestamp);
void appbase_close(struct appbase *);

void appbase_enable_progress(struct appbase *appbase, bool enable);
void appbase_enable_verbose(struct appbase *appbase, bool enable);

typedef void (* appbase_frame_cb_t) (const char *data, size_t len, void *userdata);
bool appbase_stream_loop(struct appbase *, appbase_frame_cb_t, void *);

#endif /* APPBASE_H_ */
