/*
 * client-main.c
 *
 *  Created on: 22 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <linux/videodev2.h>
#include <stdio.h>
#include <getopt.h>
#include "main.h"
#include "utils.h"
#include "appbase.h"
#include "uvc.h"
#include "window.h"

static void print_usage(const char *name)
{
	if (name) {
		printf("Usage: %s [OPTIONS] <app name> <username> <password>\n"
				"Options:\n"
				"    -d    Display debug messages\n",
				name);
	}
}

int main(int argc, char **argv)
{
	int opt;
	bool debug = false;
	struct appbase *ab;
	struct frame *frame;
	struct window *window;

	opt = getopt(argc, argv, "d");
	if (opt != -1 && opt == 'd')
		debug = true;
	else if (opt != -1)
		goto exit_help;

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

	frame = uvc_alloc_frame(320, 240, V4L2_PIX_FMT_YUYV);
	if (!frame)
		fatal("Could not allocate frame");

	window = start_window(320, 240, V4L2_PIX_FMT_YUYV);
	while (!window->is_closed()) {
		if (!appbase_fill_frame(ab, frame))
			fatal("Could not read frame");
		if (!window->render(frame))
			fatal("Could not render frame");
	}
	destroy_window(window);
	window = NULL;

	uvc_free_frame(frame);
	appbase_close(ab);
	goto exit;

exit_help:
	print_usage(argv[0]);
exit:
	return 0;
}
