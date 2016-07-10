/*
 * json-stream.c
 *
 *  Created on: 2 Jul 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <string.h>
#include "main.h"
#include "utils.h"
#include "appbase.h"
#include "json-streamer.h"

struct json_streamer {
	yajl_handle yajl;
	json_streamer_frame_cb_t frame_callback;
	void *userdata;
	struct json_streamer_state_ctx *ctx;
};

enum json_streamer_state {
	waiting,
	string_parsed,
	integer_parsed,
	usec_key_parsed,
	image_key_parsed,
	usec_key_waiting,
	image_key_waiting
};

struct json_streamer_state_ctx {
	json_streamer_frame_cb_t frame_callback;
	void *userdata;
	enum json_streamer_state cur_state;
	char *data;
	size_t len;
	long long val;
};

/*
 * yajl callbacks
 *
 * Return non-zero to let the parsing continue.
 * Return zero to signal an error (yajl_parse() will return 'yajl_status_client_canceled').
 */
static int json_streamer_new_state(struct json_streamer_state_ctx *state_ctx,
		enum json_streamer_state new_state)
{
	int retval = 1;

	switch (new_state) {
	case string_parsed:
		if (state_ctx->cur_state == image_key_waiting) {
			state_ctx->frame_callback(state_ctx->data, state_ctx->len, state_ctx->userdata);
			state_ctx->cur_state = waiting;
		}
		break;
	case integer_parsed:
		if (state_ctx->cur_state == usec_key_waiting) {
			fprintf(stderr, "Received usec: %lld\n", state_ctx->val);
			retval = 0;
			state_ctx->cur_state = waiting;
		}
		break;
	case usec_key_parsed:
		state_ctx->cur_state = usec_key_waiting;
		break;
	case image_key_parsed:
		state_ctx->cur_state = image_key_waiting;
		break;
	case image_key_waiting:
	case usec_key_waiting:
	default:
		break;
	}

	return retval;
}

static int yajl_json_integer_cb(void *ctx, long long val)
{
	struct json_streamer_state_ctx *state_ctx = ctx;

	state_ctx->val = val;
	return json_streamer_new_state(ctx, integer_parsed);
}

static int yajl_json_string_cb(void *ctx, const unsigned char *str, size_t len)
{
	struct json_streamer_state_ctx *state_ctx = ctx;

	state_ctx->data = (char *) str;
	state_ctx->len = len;
	return json_streamer_new_state(state_ctx, string_parsed);
}

static int yajl_json_map_key_cb(void *ctx, const unsigned char *key, size_t len)
{
	int retval = 1;
	if (len == 4) {
		if (strncmp((const char *) key, AB_KEY_USEC, len) == 0)
			retval = json_streamer_new_state(ctx, usec_key_parsed);
	} else if (len == 5) {
		if (strncmp((const char *) key, AB_KEY_IMAGE, len) == 0)
			retval = json_streamer_new_state(ctx, image_key_parsed);
	}

	return retval;
}

static yajl_callbacks yajl_cbs = {
		NULL,
		NULL,
		yajl_json_integer_cb,
		NULL,
		NULL,
		yajl_json_string_cb,
		NULL,
		yajl_json_map_key_cb,
		NULL,
		NULL,
		NULL
};

static void yajl_init(struct json_streamer *json, json_streamer_frame_cb_t fcb, void *userdata, bool reinit)
{
	struct json_streamer_state_ctx *ctx = ec_malloc(sizeof(struct json_streamer_state_ctx));

	ctx->cur_state = waiting;
	ctx->frame_callback = fcb;
	ctx->userdata = userdata;

	/* Save pointer so that we can free it later in json_streamer_destroy() */
	json->ctx = ctx;

	if (reinit) {
		yajl_complete_parse(json->yajl);
		yajl_free(json->yajl);
	}

	json->yajl = yajl_alloc(&yajl_cbs, NULL, ctx);
}

/*
 * JSON Streamer public API
 */
struct json_streamer *json_streamer_init(json_streamer_frame_cb_t fcb, void *userdata)
{

	struct json_streamer *json = ec_malloc(sizeof(struct json_streamer));

	if (!fcb)
		goto fail;

	json->frame_callback = fcb;
	json->userdata = userdata;
	yajl_init(json, fcb, userdata, false);

	return json;

fail:
	free(json);
	return NULL;
}

void json_streamer_destroy(struct json_streamer *json)
{
	if (json) {
		if (json->yajl)
			yajl_free(json->yajl);
		if (json->ctx)
			free(json->ctx);

		free(json);
	}
}

bool json_streamer_push(struct json_streamer *json, const unsigned char *data, size_t size)
{
	yajl_status status;

	if (!json || !json->yajl || !data || !size)
		return false;

	status = yajl_parse(json->yajl, data, size);
	if (status == yajl_status_client_canceled)
		yajl_init(json, json->frame_callback, json->userdata, true);

	return (status != yajl_status_error);
}

unsigned char *json_streamer_get_last_error(struct json_streamer *json,
		unsigned char *data, size_t size)
{
	if (!json || !json->yajl || !data || !size)
		return false;

	return yajl_get_error(json->yajl,
			1,
			data,
			size);
}

void json_streamer_free_last_error(struct json_streamer *json, unsigned char *err)
{
	if (json && err)
		yajl_free_error(json->yajl, err);
}
