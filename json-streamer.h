/*
 * json-streamer.h
 *
 *  Created on: 2 Jul 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef JSON_STREAMER_H_
#define JSON_STREAMER_H_
#include "main.h"
#include "appbase.h"

struct json_streamer;

typedef void (* json_streamer_frame_cb_t) (const char *, size_t, void *);
struct json_streamer *json_streamer_init(json_streamer_frame_cb_t, void *);
void json_streamer_destroy(struct json_streamer *);

bool json_streamer_push(struct json_streamer *json,
		const unsigned char *data,
		size_t size);

unsigned char *json_streamer_get_last_error(struct json_streamer *json,
		unsigned char *data,
		size_t size);
void json_streamer_free_last_error(struct json_streamer *json,
		unsigned char *err);

#endif /* JSON_STREAMER_H_ */
