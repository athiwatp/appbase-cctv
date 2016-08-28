/*
 * utils.c
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#include "utils.h"

#define DEFAULT_INITIAL_LEN 30
#define IS_COMMENT(tstr) (*tstr == '#')

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
	void *mem = NULL;

	if (!size)
		goto end;

	mem = calloc(1, size);
	if (!mem)
		fatal("Out of memory");

end:
	return mem;
}

void *ec_malloc_fill(size_t size, const char *data)
{
	void *mem = ec_malloc(size);
	memcpy(mem, data, size);
	return mem;
}

void *ec_realloc(void *ptr, size_t size)
{
	void *mem = NULL;

	if (!ptr)
		mem = ec_malloc(size);
	else
		mem = realloc(mem, size);

	return mem;
}

char *strdup_delim(const char *str, size_t start, size_t end)
{
	size_t len, pos = 0;
	char *dst;

	if (!str)
		return strdup("");

	len = strlen(str);
	if (start > len || end > len || start > end)
		return strdup("");

	/* Leave one extra slot for the null character */
	dst = ec_malloc(end - start + 2);

	while (start <= end)
		dst[pos++] = str[start++];
	dst[pos] = '\0';

	return dst;
}

char *strdup_trim(const char *str)
{
	size_t pos = 0, start = 0, end = 0;
	size_t len;

	if (!str)
		return NULL;

	len = strlen(str);
	while (pos < len && isblank(str[pos]))
		pos++;

	if (pos == len)
		goto empty;

	start = pos;
	pos = len - 1;
	while (pos > start && isblank(str[pos]))
		pos--;
	end = pos;

	return strdup_delim(str, start, end);

empty:
	return strdup("");
}

bool read_line(FILE *fp, char **line)
{
	size_t cur_len = DEFAULT_INITIAL_LEN;
	size_t read = 0;
	char cur_char = 0;

	if (!fp || !line || fileno(fp) == -1)
		return false;
	if (feof(fp))
		return false;

	if (*line)
		free(*line);
	*line = ec_malloc(cur_len);

loop:
	do {
		cur_char = getc(fp);
		*line[read++] = cur_char;
	} while (read < cur_len && cur_char != '\n' && !feof(fp));

	if (feof(fp) || cur_char == '\n') {
		/*
		 * In case we reached end of file without reading a final '\n',
		 * then the last character should be EOF instead of '\n',
		 * so we just replace it with '\0' as if it was an '\n'.
		 */
		*line[read - 1] = '\0';
		goto end;
	}

	/* We double the size and continue reading the current line */
	cur_len <<= 1;
	*line = ec_realloc(line, cur_len);
	goto loop;

end:
	return true;
}

bool read_section(const char *str, char **sections, unsigned int nsections, int *section_idx)
{
	bool retval = false;
	char *tstr = strdup_trim(str), *section = NULL;
	size_t len;

	if (!tstr || !sections || !nsections || !section_idx)
		goto end;
	if (IS_COMMENT(tstr))
		goto end;

	len = strlen(tstr);
	if (len == 0)
		goto end;

	if (tstr[0] != '[' && tstr[len - 1] != ']')
		goto end;

	section = strdup_delim(tstr, 0, len - 1);
	for (int i = 0; i < nsections; i++) {
		if (!strcmp(sections[i], section)) {
			*section_idx = i;
			break;
		}
	}

end:
	if (tstr)
		free(tstr);
	if (section)
		free(section);
	return retval;
}

bool read_key_and_value(const char *str, char **key, char **value)
{
	bool retval;
	size_t pos = 0, latest_nonspace = 0;
	char *tstr = strdup_trim(str);
	size_t len;

	if (!tstr || !key || !value)
		return false;
	if (IS_COMMENT(tstr))
		return false;

	len = strlen(tstr);
	if (len == 0)
		goto bail;

	while (pos < len && tstr[pos] != '=') {
		if (!isblank(tstr[pos]))
			latest_nonspace = pos;
		pos++;
	}

	if (pos == len) {
		/*
		 * This is an error, since we got to the end of the line
		 * without still finding an '=' sign.
		 */
		goto bail;
	}

	*key = strdup_delim(tstr, 0, latest_nonspace);
	pos++;

	/* Advance until next non-space */
	while (pos < len && isblank(tstr[pos]))
		pos++;

	if (pos == len)
		goto bail;

	*value = strdup_delim(tstr, pos, len - 1);
	retval = true;

	goto end;

bail:
	if (*key)
		free(key);
	if (*value)
		free(value);
	retval = false;

end:
	free(tstr);
	return retval;
}
