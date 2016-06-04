/*
 * appbase.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <curl/curl.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
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
};

/*
 * Callback for libcurl. This should fill 'buffer' with the data we want to send
 * and return the actual number of bytes written.
 * 'instream' holds user-defined data, a pointer to a 'json_internal' struct
 * in our case.
 */
static size_t reader_cb(char *buffer, size_t size, size_t nitems, void *instream)
{
	size_t written = 0, buf_len = size * nitems;
	struct json_internal *json = (struct json_internal *) instream;

	if (!instream || !json->json || json->length <= 0)
		goto end;

	written = (buf_len >= json->length ? json->length : buf_len);
	if (!buffer || buf_len <= 0) {
		written = 0;
		goto end;
	}

	memcpy(buffer, json->json, written);

end:
	return written;
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

	/* Create out base JSON object */
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
	int freed = 0;
	if (ab) {
		if (ab->curl)
			curl_easy_cleanup(ab->curl);
		if (ab->url)
			free(ab->url);
		if (ab->json)
			freed = json_object_put(ab->json);

		ab->curl = NULL;
		ab->url = NULL;
		ab->json = NULL;
	}
}

void appbase_enable_progress(struct appbase *ab, int enable)
{
	if (ab && ab->curl && (enable == 0 || enable == 1))
		curl_easy_setopt(ab->curl, CURLOPT_NOPROGRESS, !enable);
}

void appbase_enable_verbose(struct appbase *ab, int enable)
{
	if (ab && ab->curl && (enable == 0 || enable == 1)) {
		curl_easy_setopt(ab->curl, CURLOPT_VERBOSE, enable);
		curl_easy_setopt(ab->curl, CURLOPT_WRITEFUNCTION, fwrite);
	}
}

int appbase_push_frame(struct appbase *ab,
		const char *data, unsigned int length)
{
	int retval = -1;
	struct json_internal json;

	if (!ab || !ab->curl || !ab->url || !ab->json || !data || length <= 0)
		return -1;

	/* TODO
	 * This is for testing purposes only.
	 * Never treat raw image data as a string (ie. null terminated).
	 */
	/*
	 * Generate a JSON object with the format:
	 *
	 * 	{"image": "<data>"}
	 */
	json_object_object_add(ab->json,
			"image",
			json_object_new_string(data));
	json.json = json_object_to_json_string_ext(ab->json, JSON_C_TO_STRING_PLAIN);
	json.length = strlen(json.json);

	curl_easy_setopt(ab->curl, CURLOPT_URL, ab->url);
	curl_easy_setopt(ab->curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(ab->curl, CURLOPT_INFILESIZE, json.length);
	curl_easy_setopt(ab->curl, CURLOPT_READDATA, &json);
	curl_easy_setopt(ab->curl, CURLOPT_READFUNCTION, reader_cb);

	retval = curl_easy_perform(ab->curl);

	/* No need to free, json_object_put() releases the string for us */
	json.length = 0;

	return (retval == CURLE_OK);
}
