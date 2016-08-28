/*
 * config.c
 *
 *  Created on: 27 Aug 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "main.h"
#include "utils.h"

static void param_string(char **dst, const char *src)
{
	/*
	 * This is essentially strdup.
	 * But strdup uses malloc, and we want to use ec_malloc.
	 */
	size_t len = strlen(src);
	*dst = ec_malloc(len + 1);
	strncpy(*dst, src, len);
}

static bool param_int(long int *dst, const char *src)
{
	char *endptr;

	*dst = strtol(src, &endptr, 10);
	if (*endptr || *dst < 0)
		return false;

	return true;
}

static bool param_boolean(bool *dst, const char *src)
{
	if (!strcasecmp(src, "true"))
		*dst = true;
	else if (!strcasecmp(src, "false"))
		*dst = false;
	else
		return false;

	return true;
}

bool read_from_file(const char *filename,
		long int *wait_time,
		bool *debug, bool *oneshot, bool *stream, bool *jpeg,
		char **ab_app_name, char **ab_username, char **ab_password,
		char **error_msg)
{
	bool retval = true, result;
	FILE *fp = NULL;
	char *absolute_filename = NULL, *home_dir = NULL;
	char *line = NULL, *key = NULL, *value = NULL;
	char *error = NULL;
	char *sections[] = {
			"daemon",
			"appbase"
	};
#define NUM_SECTIONS	2
	int cur_section = -1;
	unsigned int cur_line = 0;

	if (!filename || !wait_time || !debug || !oneshot || !stream || !jpeg ||
			!ab_app_name || !ab_username || !ab_password)
		return false;

	if (filename[0] == '~' && filename[1] == '/') {
#ifdef _GNU_SOURCE
		asprintf(&absolute_filename, "%s/%s", home_dir, filename + 2);
#else
#error "Sorry. Non-GNU environments are not yet supported."
#endif
	}

	fp = fopen(absolute_filename == NULL ? filename : absolute_filename, "r");
	if (!fp)
		return false;

	while (read_line(fp, &line)) {
		cur_line++;

		if (read_section(line, sections, NUM_SECTIONS, &cur_section))
			continue;

		if (read_key_and_value(line, &key, &value)) {
			switch (cur_section) {
			case 0: /* daemon */
				if (!strcmp(key, "wait_time"))
					result = param_int(wait_time, value);
				else if (!strcmp(key, "oneshot"))
					result = param_boolean(oneshot, value);
				else if (!strcmp(key, "stream"))
					result = param_boolean(stream, value);
				else if (!strcmp(key, "debug"))
					result = param_boolean(debug, value);
				else if (!strcmp(key, "jpeg"))
					result = param_boolean(jpeg, value);
				else
					goto error_invalid_param;

				if (!result)
					goto syntax_error;

				break;
			case 1: /* appbase */
				if (!strcmp(key, "username"))
					param_string(ab_app_name, value);
				else if (!strcmp(key, "password"))
					param_string(ab_password, value);
				else if (!strcmp(key, "app"))
					param_string(ab_app_name, value);
				else
					goto error_invalid_param;

				break;
			default:
				goto error_invalid_section;
			}

			free(key);
			free(value);
		} else {
			goto syntax_error;
		}
	}

	goto end;

error_invalid_param:
	asprintf(&error, "Invalid parameter: %s (sect. '%s')\n", key, sections[cur_section]);
	goto end;

error_invalid_section:
	asprintf(&error, "Syntax error at line %d: Invalid section\n", cur_line);
	goto end;

syntax_error:
	asprintf(&error, "Syntax error at line %d\n", cur_line);

end:
	if (line)
		free(line);
	if (key)
		free(key);
	if (value)
		free(value);

	fclose(fp);

	if (error) {
		if (error_msg)
			*error_msg = error;
		retval = false;
	}

	return retval;
}

bool read_from_cmdline(int argc, char **argv,
		long int *wait_time,
		bool *debug, bool *oneshot, bool *stream, bool *jpeg,
		char **ab_app_name, char **ab_username, char **ab_password)
{
	bool retval = true;
	int opt;

	if (argc <= 1 || !argv || !wait_time || !debug || !oneshot || !stream || !jpeg)
		return false;

	while (retval && (opt = getopt(argc, argv, "w:dsSj")) != -1) {
		switch (opt) {
		case 'w':
			param_int(wait_time, optarg);
			break;
		case 'd':
			*debug = true;
			break;
		case 's':
			*oneshot = true;
			break;
		case 'S':
			*stream = true;
			break;
		case 'j':
			*jpeg = true;
			break;
		default:
			retval = false;
			break;
		}
	}

	if (!retval)
		goto end;

	while (argc - optind > 0) {
		/*
		 * Still more params remaining.
		 * These should be the Appbase app name, username and password.
		 */
		switch (argc - optind) {
		case 1:
			*ab_app_name = argv[optind];
			break;
		case 2:
			*ab_username = argv[optind];
			break;
		case 3:
			*ab_password = argv[optind];
			break;
		default:
			break;
		}

		optind++;
	}

end:
	return retval;
}
