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

static char *create_unique_filename()
{
	struct stat s;
#define COUNT_LIMIT 9
	unsigned int count = 0;
	char filename_template[] = "picture";
	size_t size = sizeof(filename_template) + 2;
	char *filename = ec_malloc(size);

	do {
		snprintf(filename, size, "%s.%d", filename_template, count++);
	} while (stat(filename, &s) >= 0 && count <= COUNT_LIMIT);

	if (count > COUNT_LIMIT)
		snprintf(filename, size, "%s.%d", filename_template, 0);

	return filename;
#undef COUNT_LIMIT
}

static void write_to_disk(const char *data, size_t size)
{
	FILE *f;
	char *filename = create_unique_filename();

	f = fopen(filename, "w");
	if (f) {
		fwrite(data, size, 1, f);
		fclose(f);
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
				"    -s             Take one single shot and exit\n",
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

static void run(struct camera *camera, struct appbase *ab, bool debug)
{
	if (uvc_capture_frame(camera)) {
			if (!appbase_push_frame(ab,
					camera->frame, camera->frame_bytes_used,
					&camera->capture_time))
				fprintf(stderr, "ERROR: Could not send frame\n");

		if (debug)
			write_to_disk(camera->frame, camera->frame_bytes_used);

		memset(camera->frame, 0, camera->frame_size);
		camera->frame_bytes_used = 0;
	} else {
		fprintf(stderr, "ERROR: Could not capture frame\n");
	}

}

int main(int argc, char **argv)
{
	int opt;
	char *endptr;
	long int wait_time = DEFAULT_WAIT_TIME;
	bool debug = false, oneshot = false;
	struct sigaction sig;
	struct appbase *ab;
	struct camera *camera;

	/* Parse command-line options */
	while ((opt = getopt(argc, argv, "w:ds")) != -1) {
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
			argv[optind + 2]);	// password
	if (!ab)
		fatal("Could not log into Appbase");

	if (debug) {
		appbase_enable_progress(ab, true);
		appbase_enable_verbose(ab, true);
	}

	camera = uvc_open();
	if (!camera)
		fatal("Could not find any camera for capturing pictures");

	/*
	 * We'll capture 320x240 images in YUYV format. YUYV encodes each pixel in 2 bytes,
	 * so the size of the frames will be 320 x 240 x 2 = 153,600 bytes (~153 KB).
	 * All the frames will have exactly the same size (unless we, at some point, change
	 * the dimensions or the format), so we can allocate a fixed chunk of memory and re-use it
	 * for each frame.
	 *
	 * Frame format:
	 * +-------------+----------+-------------+----------+
	 * |    Byte 0   |  Byte 1  |    Byte 2   |  Byte 3  |
	 * +-------------+----------+-------------+----------+
	 * | Y (pixel 0) | U (both) | Y (pixel 1) | V (both) |
	 * +-------------+----------+-------------+----------+
	 */
	camera->width = 320;
	camera->height = 240;
	camera->format = V4L2_PIX_FMT_YUYV;
	if (!uvc_alloc_frame(camera))
		fatal("Could not allocate enough memory for frames");

	if (oneshot) {
		run(camera, ab, debug);
		goto end;
	}

	while (!IS_STOPPED()) {
		run(camera, ab, debug);

		/*
		 * sleep(3) should not interfere with our signal handlers,
		 * unless we're also handling SIGALRM
		 */
		sleep(wait_time);
	}

end:
	uvc_close(camera);
	appbase_close(ab);

	return 0;
}
