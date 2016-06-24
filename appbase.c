/*
 * appbase.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <curl/curl.h>
#include <json-c/json_object.h>
#include <modp_b64.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "main.h"
#include "utils.h"

#define APPBASE_API_URL "scalr.api.appbase.io"
#define APPBASE_PATH	"pic/1"

struct appbase {
	char *url;
	CURL *curl;
	json_object *json;
};

static char *appbase_generate_url(const char *app_name,
		const char *username, const char *password)
{
	char *url = NULL;

	if (!app_name || !username || !password)
		goto fatal;

#ifdef _GNU_SOURCE
	if (asprintf(&url, "https://%s:%s@%s/%s/%s",
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

struct json_internal {
	const char *json;
	size_t length;
	off_t offset;
};

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
 * Writer callback for libcurl. This is just a sink to prevent libcurl from printing
 * data received from the server on screen (when debug mode is disabled). We just set
 * CURLOPT_WRITEFUNCTION to this function, so that data to be written will be passed to us
 * instead.
 */
static size_t writer_cb(char *ptr, size_t size, size_t nmemb, void *userdada)
{
	return size * nmemb;
}

/*
 * TODO
 * Socket creation and connection should also be handled here.
 * libcurl might open and close an entirely new connection for every PUT, which we don't want.
 */
struct appbase *appbase_open(const char *app_name,
		const char *username, const char *password)
{
 	struct appbase *ab =
			ec_malloc(sizeof(struct appbase));

 	/* Initialize libcurl, and set up our Appbase REST URL */
	ab->curl = curl_easy_init();
	if (!ab->curl)
		goto fatal;

	curl_easy_setopt(ab->curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(ab->curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, writer_cb);

	ab->url = appbase_generate_url(app_name, username, password);
	if (!ab->url)
		goto fatal;

	/* Create our base JSON object */
	ab->json = json_object_new_object();
	if (!ab->json)
		goto fatal;

	return ab;

fatal:
	if (ab->curl)
		curl_easy_cleanup(ab->curl);
	if (ab->url)
		free(ab->url);
	free(ab);

	return NULL;
}

void appbase_close(struct appbase *ab)
{
	if (ab) {
		if (ab->curl)
			curl_easy_cleanup(ab->curl);
		if (ab->url)
			free(ab->url);
		if (ab->json)
			json_object_put(ab->json);

		ab->curl = NULL;
		ab->url = NULL;
		ab->json = NULL;

		free(ab);
	}
}

void appbase_enable_progress(struct appbase *ab, bool enable)
{
	if (ab && ab->curl && (enable == 0 || enable == 1))
		curl_easy_setopt(ab->curl, CURLOPT_NOPROGRESS, !enable);
}

void appbase_enable_verbose(struct appbase *ab, bool enable)
{
	if (ab && ab->curl && (enable == 0 || enable == 1)) {
		curl_easy_setopt(ab->curl, CURLOPT_VERBOSE, enable);
		curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, fwrite);
	}
}

bool appbase_push_frame(struct appbase *ab,
		const char *data, size_t length,
		struct timeval *timestamp)
{
	CURLcode response_code;
	struct json_internal json;
	size_t b64_size = 0;
	char *b64_data;

	if (!ab || !ab->curl || !ab->url || !ab->json || !data || !length || !timestamp)
		return false;

	/* Transform raw frame data into base64 */
	b64_size = modp_b64_encode_len(length);
	b64_data = ec_malloc(b64_size);
	if (modp_b64_encode(b64_data, data, length) == -1)
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
	json_object_object_add(ab->json, "image", json_object_new_string(b64_data));
	json_object_object_add(ab->json, "sec", json_object_new_int64(timestamp->tv_sec));
	json_object_object_add(ab->json, "usec", json_object_new_int64(timestamp->tv_usec));

	json.json = json_object_to_json_string_ext(ab->json, JSON_C_TO_STRING_PLAIN);
	json.length = strlen(json.json);
	json.offset = 0;

	curl_easy_setopt(ab->curl, CURLOPT_URL, ab->url);
	curl_easy_setopt(ab->curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_INFILESIZE, json.length);
	curl_easy_setopt(ab->curl, CURLOPT_READDATA, &json);
	curl_easy_setopt(ab->curl, CURLOPT_READFUNCTION, reader_cb);

	response_code = curl_easy_perform(ab->curl);

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

/* TODO implement this */
bool appbase_fill_frame(struct frame *f)
{
	return true;
}
