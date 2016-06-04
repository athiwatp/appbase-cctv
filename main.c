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
#include "utils.h"
#include "appbase.h"
#include "uvc.h"

#define DEFAULT_WAIT_TIME	5

int stop;
#define SHOULD_STOP(v) (stop = v)
#define IS_STOPPED()   (stop)

void print_usage_and_exit(const char *name)
{
	if (name) {
		printf("Usage: %s [OPTIONS] <app name> <username> <password>\n"
				"Options:\n"
				"    -w secs        Sleep this amount of seconds between shots\n"
				"    -d             Display debug messages\n",
				name);
	}
	exit(1);
}

void sighandler(int s)
{
	char *signame;

	switch (s) {
	case SIGINT:
		signame = "SIGINT";
		break;
	case SIGQUIT:
		signame = "SIGQUIT";
		break;
	case SIGKILL:
		signame = "SIGKILL";
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

int main(int argc, char **argv)
{
	int opt;
	char *endptr;
	long int wait_time = DEFAULT_WAIT_TIME;
	int debug = 0;
	char *app_name, *username, *password;
	struct sigaction sig;
	struct appbase *ab;
	struct camera *camera;
	struct frame *frame;

	/* Parse command-line options */
	while ((opt = getopt(argc, argv, "w:d")) != -1) {
		switch (opt) {
		case 'w':
			wait_time = strtol(optarg, &endptr, 10);
			if (*endptr || wait_time < 0)
				print_usage_and_exit(argv[0]);
			break;
		case 'd':
			debug = 1;
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
	sigaction(SIGKILL, &sig, NULL);
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
		appbase_enable_progress(ab, 1);
		appbase_enable_verbose(ab, 1);
	}

	camera = uvc_open();
	if (!camera)
		fatal("Could not find any camera for capturing pictures");

	while (!IS_STOPPED()) {
		frame = uvc_capture_frame();
		if (!appbase_push_frame(ab, frame->data, frame->length))
			fprintf(stderr, "ERROR: Could not send frame\n");
		free(frame);

		/* sleep(3) should not interfere with our signal handlers, unless we're also handling SIGALRM */
		sleep(wait_time);
	}

	uvc_close(camera);
	appbase_close(ab);

	return 0;
}
