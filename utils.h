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
#include "main.h"

/* This typically runs as a daemon. Tweak this as desired. */
#define SYSLOG_FACILITY 	LOG_DAEMON

/* Memory management functions */
void fatal(const char *message);
void *ec_malloc(size_t size);
void *ec_malloc_fill(size_t size, const char *data);
void *ec_realloc(void *ptr, size_t size);

/* String management functions */
bool read_line(FILE *fp, char **line);
char *strdup_delim(const char *str, size_t start, size_t end);
char *strdup_trim(const char *str);
bool read_section(const char *str, char **sections, unsigned int nsections, int *section_idx);
bool read_key_and_value(const char *str, char **key, char **value);

#endif /* UTILS_H_ */
