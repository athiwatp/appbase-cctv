/*
 * main.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "appbase.h"

#define DEFAULT_WAIT_TIME	5

int stop;
#define SHOULD_STOP(v) (stop = v)
#define IS_STOPPED()   (stop)

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
		syslog(SYSLOG_FACILITY | LOG_INFO, "Received %s. Exiting.", signame);
	else
		syslog(SYSLOG_FACILITY | LOG_INFO, "Received %d. Exiting.", s);

	/* Stop! */
	SHOULD_STOP(1);
}

int main(int argc, char **argv)
{
	int opt;
	char *endptr;
	long int wait_time = DEFAULT_WAIT_TIME;
	int debug = 0;

	struct sigaction sig;

	struct appbase *ab;

	/* Parse command-line options */
	while ((opt = getopt(argc, argv, "w:d")) != -1) {
		switch (opt) {
		case 'w':
			wait_time = strtol(optarg, &endptr, 10);
			if (*endptr || wait_time < 0)
				fatal("Invalid value: -w takes a positive integer");
			break;
		case 'd':
			debug = 1;
			break;
		default:
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

	/* Set up Appbase handle */

	/*ab = appbase_open(app_name, username, password);
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
		appbase_push_frame(ab, frame);

		wait_for(wait_time);
	}

	uvc_close(camera);
	appbase_close(ab);*/

	return 0;
}
