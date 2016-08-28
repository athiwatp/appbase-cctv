/*
 * config.h
 *
 *  Created on: 28 Aug 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include "main.h"

bool read_from_cmdline(int argc, char **argv,
		long int *wait_time,
		bool *debug, bool *oneshot, bool *stream, bool *jpeg,
		char **ab_app_name, char **ab_username, char **ab_password);

bool read_from_file(const char *filename,
		long int *wait_time,
		bool *debug, bool *oneshot, bool *stream, bool *jpeg,
		char **ab_app_name, char **ab_username, char **ab_password,
		char **error_msg);

#endif /* CONFIG_H_ */
