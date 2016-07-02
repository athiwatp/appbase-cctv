/*
 * json-stream.c
 *
 *  Created on: 2 Jul 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <modp_b64.h>
#include <string.h>
#include "main.h"
#include "utils.h"
#include "appbase.h"

struct json_streamer {
	yajl_handle yajl;
	frame_cb frame_callback;
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
	frame_cb frame_callback;
	enum json_streamer_state cur_state;
	char *data;
	size_t len;
	long long val;
};

static void json_streamer_post_image_data(frame_cb frame_callback, const char *data, size_t len)
{
	struct frame f;

	size_t image_len = modp_b64_decode_len(len);
	char *image = ec_malloc(image_len);
	f.frame_size = image_len;

	image_len = modp_b64_decode(image, data, len);
	f.frame_data = (unsigned char *) image;
	f.frame_bytes_used = image_len;

	if (image_len != -1) {
		frame_callback(&f);
//		fprintf(stderr, "Image decoded (b64 len: %d, decoded len: %d)\n", len, image_len);
	} else {
//		fprintf(stderr, "ERROR decoding image\n");
	}

	free(image);
}

/*
 * yajl callbacks
 *
 * Return non-zero to let the parsing continue.
 * Return zero to signal an error.
 */
static int json_streamer_new_state(struct json_streamer_state_ctx *state_ctx,
		enum json_streamer_state new_state)
{
	int retval = 1;

	switch (new_state) {
	case string_parsed:
		if (state_ctx->cur_state == image_key_waiting) {
			json_streamer_post_image_data(state_ctx->frame_callback, state_ctx->data, state_ctx->len);
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

static void yajl_init(struct json_streamer *json, frame_cb fcb, bool reinit)
{
	struct json_streamer_state_ctx *ctx = ec_malloc(sizeof(struct json_streamer_state_ctx));

	ctx->cur_state = waiting;
	ctx->frame_callback = fcb;

	if (reinit) {
		yajl_complete_parse(json->yajl);
		yajl_free(json->yajl);
	}

	json->yajl = yajl_alloc(&yajl_cbs, NULL, ctx);
}

/*
 * JSON Streamer public API
 */
struct json_streamer *json_streamer_init(frame_cb fcb)
{

	struct json_streamer *json = ec_malloc(sizeof(struct json_streamer));

	if (!fcb)
		goto fail;

	json->frame_callback = fcb;
	yajl_init(json, fcb, false);

	return json;

fail:
	free(json);
	return NULL;
}

bool json_streamer_push(struct json_streamer *json, const unsigned char *data, size_t size)
{
	yajl_status status;

	if (!json || !json->yajl || !data || !size)
		return false;

	status = yajl_parse(json->yajl, data, size);
	if (status == yajl_status_client_canceled)
		yajl_init(json, json->frame_callback, true);

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
