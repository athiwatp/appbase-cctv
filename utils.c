/*
 * mem.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <syslog.h>
#include <stdlib.h>
#include <malloc.h>
#include "utils.h"

void fatal(const char *message)
{
//	syslog(SYSLOG_FACILITY | LOG_ERR, "FATAL ERROR: %s", message);
//	syslog(SYSLOG_FACILITY | LOG_ERR, "Stopping");

	fprintf(stderr, "FATAL ERROR: %s\n", message);
	fprintf(stderr, "Stopping.\n");

	exit(1);
}

void *ec_malloc(size_t size)
{
	void *mem = calloc(1, size);
	if (!mem)
		fatal("Out of memory");

	return mem;
}
