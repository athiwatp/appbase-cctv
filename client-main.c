/*
 * client-main.c
 *
 *  Created on: 22 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <linux/videodev2.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include "main.h"
#include "utils.h"
#include "appbase.h"
#include "uvc.h"
#include "window.h"
#include "cb.h"

#define CB_LEN 5

static struct appbase *ab = NULL;

static void print_usage(const char *name)
{
	if (name) {
		printf("Usage: %s [OPTIONS] <app name> <username> <password>\n"
				"Options:\n"
				"    -d    Display debug messages\n",
				name);
	}
}

/* TODO implement this
 * Should render frame in SDL window.
 * Frame 'f' might be NULL. This means that a frame was retrieved from Appbase,
 * but that we were unable to decode it.
 */
static void frame_callback(const char *data, size_t len, void *userdata)
{
//	struct window *w = userdata;
	struct cb *cb = userdata;
	if (data && len && cb) {
		/* 'data' will be freed in the main thread */
		cb_append(cb, data, len);
		fprintf(stderr, "Image decoded (decoded len: %d)\n", len);
	} else {
		fprintf(stderr, "ERROR decoding image\n");
	}
}

static void *thread_loop(void *ptr)
{
	if (!appbase_stream_loop(ab, frame_callback, ptr))
		fprintf(stderr, "ERROR: Could not stream from Appbase\n");
	return NULL;
}

int main(int argc, char **argv)
{
	int opt;
	bool debug = false;
	enum frame_format format = FRAME_FORMAT_YUYV;
	struct window *window;
	struct frame frame;
	struct cb *cb;
	pthread_t thread;
	pthread_attr_t thread_attr;

	while ((opt = getopt(argc, argv, "dj")) != -1) {
		switch (opt) {
		case 'd':
			debug = true;
			break;
		case 'j':
			format = FRAME_FORMAT_JPEG;
			break;
		default:
			goto exit_help;
		}
	}

	if (argc - optind < 3)
		goto exit_help;

	ab = appbase_open(
			argv[optind],		// app name
			argv[optind + 1],	// username
			argv[optind + 2],	// password
			true);			// enable streaming
	if (!ab)
		fatal("Could not log into Appbase");

	if (debug) {
		appbase_enable_progress(ab, true);
		appbase_enable_verbose(ab, true);
	}

	cb = cb_start(CB_LEN);
	window = start_window(320, 240, format);

	/*
	 * Run the Appbase loop in a separate thread.
	 * In the main thread, we wait until the user closes the window.
	 */
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread, &thread_attr, thread_loop, (void *) cb);

	while (!window_is_closed()) {
		if (cb_try_next(cb, (const char **) &frame.frame_data, &frame.frame_bytes_used)) {
			window_render_frame(window, &frame);
			free(frame.frame_data);
		}
	}

	appbase_close(ab);

	destroy_window(window);
	cb_destroy(cb);

	goto exit;

exit_help:
	print_usage(argv[0]);
exit:
	return 0;
}
