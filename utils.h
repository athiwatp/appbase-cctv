/*
 * utils.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <malloc.h>	/* so that including "utils.h" alone is enough */
#include <stddef.h>

/* This typically runs as a daemon. Tweak this as desired. */
#define SYSLOG_FACILITY 	LOG_DAEMON

void fatal(const char *message);
void *ec_malloc(size_t size);
void *ec_malloc_fill(size_t size, const char *data);

#endif /* UTILS_H_ */
