/*
 * appbase.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#ifdef USING_LIBCURL
#include <curl/curl.h>
#endif
#include <json-c/json_object.h>
#include <modp_b64.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "main.h"
#include "utils.h"
#include "frame.h"
#include "json-streamer.h"
#include "appbase.h"

#define APPBASE_API_URL "scalr.api.appbase.io"
#define APPBASE_PATH	"pic/1"

struct appbase {
	char *url;
#ifdef USING_LIBCURL
	CURL *curl;
#endif
	json_object *json;
};

static char *appbase_generate_url(const char *app_name,
		const char *username, const char *password,
		bool streaming)
{
	char *url = NULL;

	if (!app_name || !username || !password)
		goto fatal;

#ifdef _GNU_SOURCE
	if (asprintf(&url, (streaming ? "https://%s:%s@%s/%s/%s/?stream=true" : "https://%s:%s@%s/%s/%s"),
			username, password,
			APPBASE_API_URL,
			app_name,
			APPBASE_PATH) == -1)
		goto fatal;
#else
#error "Sorry. Non-GNU environments are not yet supported."
#endif

	return url;

fatal:
	return NULL;
}

/*
 * I know field 'json' is const char*. It could be char* as well.
 * But we're using the same pointer both to read and to write,
 * and so I prefer to keep it const and let the compiler warn me
 * when I write data when I should only read it (because that signals
 * poor coding practice, and I'm probably clobbering something important),
 * and tell it explicitly that I know it will go well when writing.
 *
 * So in 'reader_cb', we make sure we only read.
 * In 'writer_cb' we make explicit casting to (char *) to skip
 * compiler warnings.
 *
 * I've seen tricks such as this: http://stackoverflow.com/questions/8836418/is-const-casting-via-a-union-undefined-behaviour
 * But don't seem to make much difference in practice.
 */
struct json_internal {
	const char *json;
	size_t length;
	off_t offset;
	struct json_streamer *json_streamer;
	appbase_frame_cb_t frame_callback;
	void *userdata;
};

#ifdef USING_LIBCURL
/*
 * Callback for libcurl. This should fill 'buffer' with the data we want to send
 * and return the actual number of bytes written.
 * 'instream' holds user-defined data, a pointer to a 'json_internal' struct
 * in our case.
 */
static size_t reader_cb(char *buffer, size_t size, size_t nitems, void *instream)
{
	size_t should_write = size * nitems;
	struct json_internal *json = (struct json_internal *) instream;

	if (!instream || !json->json || json->length <= 0 || should_write <= 0)
		return CURL_READFUNC_ABORT;

	if (json->offset >= json->length) {
		/*
		 * There's no more data to send - we've sent it all.
		 * Tell libcurl we're done.
		 */
		return 0;
	}

	if (json->offset + should_write > json->length)
		should_write = json->length - json->offset;

	memcpy(buffer, json->json + json->offset, should_write);
	json->offset += should_write;

	return should_write;
}

/*
 * Writer callback for libcurl. Serves two purposes.
 *
 * When 'userdata' == NULL, this is just a sink to prevent libcurl from printing
 * data received from the server on screen (when debug mode is disabled). We just set
 * CURLOPT_WRITEFUNCTION to this function, so that data to be written will be passed
 * to us instead. This is handled by appbase_push_frame().
 *
 * When 'userdata' != NULL, then it should be a struct json_internal. This means cURL
 * issued a GET request, and 'ptr' holds the response body of the server, which we should
 * store into the json_internal struct. This is handled by appbase_fill_frame().
 */
static size_t writer_cb(unsigned char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct json_internal *json = NULL;
	size_t ttl_size = size * nmemb;
	unsigned char *err = NULL;

	if (!userdata || !ttl_size || !ptr)
		goto end;

	json = (struct json_internal *) userdata;
	if (!json_streamer_push(json->json_streamer, ptr, ttl_size)) {
		err = json_streamer_get_last_error(json->json_streamer, ptr, ttl_size);
		fprintf(stderr, "JSON ERROR: %s\n", err);
		json_streamer_free_last_error(json->json_streamer, err);
	}

end:
	return ttl_size;
}
#endif

static void frame_callback(const char *frame_data, size_t len, void *userdata)
{
	size_t image_len;
	char *image;
	struct json_internal *json = userdata;

	if (!json)
		return;

	if (frame_data && len) {
		/* 'image' should be freed by the client */
		image_len = modp_b64_decode_len(len);
		image = ec_malloc(image_len);
		image_len = modp_b64_decode(image, frame_data, len);

		if (image_len != -1)
			json->frame_callback(image, image_len, json->userdata);

		/* Success! */
		return;
	}

	/*
	 * Failure
	 * If image decoding failed, call frame_callback() with a NULL argument,
	 * to let the client know about the error.
	 */
	json->frame_callback(NULL, 0, json->userdata);

}

void appbase_close(struct appbase *ab)
{
	if (ab) {
#ifdef USING_LIBCURL
		if (ab->curl)
			curl_easy_cleanup(ab->curl);
		ab->curl = NULL;
#endif
		if (ab->url)
			free(ab->url);
		if (ab->json)
			json_object_put(ab->json);

		ab->url = NULL;
		ab->json = NULL;

		free(ab);
	}
}

/*
 * libcurl keeps the socket open when possible
 */
struct appbase *appbase_open(const char *app_name,
		const char *username, const char *password,
		bool enable_streaming)
{
	struct appbase *ab =
			ec_malloc(sizeof(struct appbase));

#ifdef USING_LIBCURL
 	/* Initialize libcurl, and set up our Appbase REST URL */
	ab->curl = curl_easy_init();
	if (!ab->curl)
		goto fatal;

	curl_easy_setopt(ab->curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(ab->curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, writer_cb);
	curl_easy_setopt(ab->curl, CURLOPT_WRITEDATA, NULL);
#endif

	ab->url = appbase_generate_url(app_name, username, password, enable_streaming);
	if (!ab->url)
		goto fatal;

	/* Create our base JSON object */
	ab->json = json_object_new_object();
	if (!ab->json)
		goto fatal;

	return ab;

fatal:
	appbase_close(ab);
	return NULL;
}

void appbase_enable_progress(struct appbase *ab, bool enable)
{
#ifdef USING_LIBCURL
	if (ab && ab->curl && (enable == 0 || enable == 1))
		curl_easy_setopt(ab->curl, CURLOPT_NOPROGRESS, !enable);
#endif
}

void appbase_enable_verbose(struct appbase *ab, bool enable)
{
#ifdef USING_LIBCURL
	if (ab && ab->curl && (enable == 0 || enable == 1)) {
		curl_easy_setopt(ab->curl, CURLOPT_VERBOSE, enable);
		curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, fwrite);
		curl_easy_setopt(ab->curl, CURLOPT_WRITEDATA, stderr);
	}
#endif
}

bool appbase_push_frame(struct appbase *ab,
		const unsigned char *data, size_t length,
		struct timeval *timestamp)
{
	CURLcode response_code;
	struct json_internal json;
	size_t b64_size = 0;
	char *b64_data;

	if (!ab ||
#ifdef USING_LIBCURL
			!ab->curl ||
#endif
			!ab->url || !ab->json || !data || !length || !timestamp)
		return false;

	/* Transform raw frame data into base64 */
	b64_size = modp_b64_encode_len(length);
	b64_data = ec_malloc(b64_size);
	if (modp_b64_encode(b64_data, (char *) data, length) == -1)
		return false;

	/*
	 * Generate a JSON object with the format:
	 *
	 * 	{
	 * 		"image": "<data>",
	 * 		"sec": "<seconds>",
	 * 		"usec": "<milliseconds>"
	 * 	}
	 */
	json_object_object_add(ab->json, AB_KEY_IMAGE, json_object_new_string(b64_data));
	json_object_object_add(ab->json, AB_KEY_SEC, json_object_new_int64(timestamp->tv_sec));
	json_object_object_add(ab->json, AB_KEY_USEC, json_object_new_int64(timestamp->tv_usec));

	json.json = json_object_to_json_string_ext(ab->json, JSON_C_TO_STRING_PLAIN);
	json.length = strlen(json.json);
	json.offset = 0;

#ifdef USING_LIBCURL
	curl_easy_setopt(ab->curl, CURLOPT_URL, ab->url);
	curl_easy_setopt(ab->curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_INFILESIZE, json.length);
	curl_easy_setopt(ab->curl, CURLOPT_READDATA, &json);
	curl_easy_setopt(ab->curl, CURLOPT_READFUNCTION, reader_cb);

	response_code = curl_easy_perform(ab->curl);
#endif

	/*
	 * No need to free the JSON string.
	 * We call json_object_put() on the root JSON object in appbase_close(),
	 * and it will release the whole JSON object, including this string
	 * for us.
	 */
	json.length = 0;
	json.offset = 0;
	free(b64_data);

	return (response_code == CURLE_OK);
}

bool appbase_stream_loop(struct appbase *ab, appbase_frame_cb_t fcb, void *userdata)
{
	CURLcode response_code;
	struct json_internal json_response;

	if (!ab ||
#ifdef USING_LIBCURL
			!ab->curl ||
#endif
			!fcb)
		return false;

	json_response.json = NULL;
	json_response.length = 0;
	json_response.offset = 0;
	json_response.frame_callback = fcb;
	json_response.userdata = userdata;
	json_response.json_streamer = json_streamer_init(frame_callback, &json_response);

	if (!json_response.json_streamer)
		return false;

#ifdef USING_LIBCURL
	curl_easy_setopt(ab->curl, CURLOPT_URL, ab->url);
	curl_easy_setopt(ab->curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, writer_cb);
	curl_easy_setopt(ab->curl, CURLOPT_WRITEDATA, &json_response);

	/*
	 * Here, curl_easy_perform() should block until the remote host closes the connection,
	 * or we call curl_easy_cleanup() (this happens in appbase_close()).
	 */
	response_code = curl_easy_perform(ab->curl);
#endif

	/* Clean up */
	json_streamer_destroy(json_response.json_streamer);

	return (response_code == CURLE_OK);
}
