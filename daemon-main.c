/*
 * main.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include "utils.h"
#include "appbase.h"
#include "uvc.h"

#define DEFAULT_WAIT_TIME	5

int stop;
#define SHOULD_STOP(v) (stop = v)
#define IS_STOPPED()   (stop)

static char *create_debug_filename()
{
#define COUNT_LIMIT 9
	static unsigned int count = 0;
	char filename_template[] = "picture";
	size_t size = sizeof(filename_template) + 2;
	char *filename = ec_malloc(size);

	snprintf(filename, size, "%s.%d", filename_template, count++);

	if (count > COUNT_LIMIT)
		count = 0;

	return filename;
#undef COUNT_LIMIT
}

static void write_to_disk(const unsigned char *data, size_t size)
{
	FILE *f;
	char *filename = create_debug_filename();

	f = fopen(filename, "w");
	if (f) {
		fwrite(data, size, 1, f);
		fclose(f);

		fprintf(stderr, "DEBUG: Frame written to file '%s'\n", filename);
	}

	free(filename);
}

static void print_usage_and_exit(const char *name)
{
	if (name) {
		printf("Usage: %s [OPTIONS] <app name> <username> <password>\n"
				"Options:\n"
				"    -w secs        Sleep this amount of seconds between shots\n"
				"    -d             Display debug messages\n"
				"    -s             Take one single shot and exit\n"
				"    -j             Convert frames to JPEG\n"
				"    -S             Stream as fast as possible\n",
				name);
	}
	exit(1);
}

static void sighandler(int s)
{
	char *signame;

	switch (s) {
	case SIGINT:
		signame = "SIGINT";
		break;
	case SIGQUIT:
		signame = "SIGQUIT";
		break;
	case SIGTERM:
		signame = "SIGTERM";
		break;
	case SIGABRT:
		signame = "SIGABRT";
		break;
	case SIGTRAP:
		signame = "SIGTRAP";
		break;
	default:
		signame = NULL;
		break;
	}

	if (signame)
		fprintf(stderr, "Received %s. Exiting.", signame);
	else
		fprintf(stderr, "Received %d. Exiting.", s);

	/* Stop! */
	SHOULD_STOP(1);
}

static void do_stream(struct appbase *ab, bool jpeg)
{
	struct camera *c;

	c = uvc_open();
	if (!c)
		fatal("Could not find any camera for capturing pictures");

	c->frame = uvc_alloc_frame(320, 240, V4L2_PIX_FMT_YUYV);
	if (!c->frame)
		fatal("Could not allocate enough memory for frames");

	if (!uvc_init(c))
		fatal("Could not start camera for streaming");

	while (!IS_STOPPED() && uvc_capture_frame(c)) {
		if (jpeg)
			frame_convert_yuyv_to_jpeg(c->frame);
		if (!appbase_push_frame(ab,
				c->frame->frame_data, c->frame->frame_bytes_used,
				&c->frame->capture_time)) {
			fprintf(stderr, "ERROR: Could not capture frame\n");
			break;
		}

		c->frame->frame_bytes_used = 0;
	}

	uvc_close(c);
}

void do_capture(struct appbase *ab, unsigned int wait_time, bool oneshot, bool jpeg, bool debug)
{
	struct camera *c;
	struct frame *f;

	while (!IS_STOPPED()) {
		c = uvc_open();
		if (!c)
			fatal("Could not find any camera for capturing pictures");

		c->frame = uvc_alloc_frame(320, 240, V4L2_PIX_FMT_YUYV);
		if (!c->frame)
			fatal("Could not allocate enough memory for frames");

		if (!uvc_init(c))
			fatal("Could not start camera for streaming");

		if (uvc_capture_frame(c)) {
			f = c->frame;
			if (jpeg)
				frame_convert_yuyv_to_jpeg(f);
			if (!appbase_push_frame(ab,
					f->frame_data, f->frame_bytes_used,
					&f->capture_time))
				fprintf(stderr, "ERROR: Could not send frame\n");

			if (debug)
				write_to_disk(f->frame_data, f->frame_bytes_used);

			memset(f->frame_data, 0, f->frame_size);
			f->frame_bytes_used = 0;
		} else {
			fprintf(stderr, "ERROR: Could not capture frame\n");
		}

		uvc_close(c);

		if (oneshot)
			break;
		/*
		 * sleep(3) should not interfere with our signal handlers,
		 * unless we're also handling SIGALRM
		 */
		sleep(wait_time);
	}

}

int main(int argc, char **argv)
{
	int opt;
	char *endptr;
	long int wait_time = DEFAULT_WAIT_TIME;
	bool debug = false, oneshot = false, stream = false, jpeg = false;
	struct sigaction sig;
	struct appbase *ab;

	/* Parse command-line options */
	while ((opt = getopt(argc, argv, "w:dsSj")) != -1) {
		switch (opt) {
		case 'w':
			wait_time = strtol(optarg, &endptr, 10);
			if (*endptr || wait_time < 0)
				print_usage_and_exit(argv[0]);
			break;
		case 'd':
			debug = true;
			break;
		case 's':
			oneshot = true;
			break;
		case 'S':
			stream = true;
			break;
		case 'j':
			jpeg = true;
			break;
		default:
			print_usage_and_exit(argv[0]);
			break;
		}
	}

	/* Set signal handlers and set stop condition to zero */
	SHOULD_STOP(0);

	memset(&sig, 0, sizeof(sig));

	sig.sa_handler = sighandler;
	sig.sa_flags = SA_RESETHAND;
	sigemptyset(&sig.sa_mask);

	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGQUIT, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);
	sigaction(SIGABRT, &sig, NULL);
	sigaction(SIGTRAP, &sig, NULL);

	/* Set up Appbase handle
	 * We need the app name, username and password to build the REST URL, and these
	 * should came now as parameters. We expect optind to point us to the first one.
	 */
	if (argc - optind < 3)
		print_usage_and_exit(argv[0]);

	ab = appbase_open(
			argv[optind],		// app name
			argv[optind + 1],	// username
			argv[optind + 2],	// password
			false);			// streaming off
	if (!ab)
		fatal("Could not log into Appbase");

	if (debug) {
		appbase_enable_progress(ab, true);
		appbase_enable_verbose(ab, true);
	}

	if (stream)
		do_stream(ab, jpeg);
	else
		do_capture(ab, wait_time, oneshot, jpeg, debug);

	appbase_close(ab);

	return 0;
}
