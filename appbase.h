/*
 * appbase.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef APPBASE_H_
#define APPBASE_H_
#include <stdint.h>

struct appbase;

struct appbase *appbase_open(const char *app_name,
		const char *username,
		const char *password);
int8_t appbase_push_frame(struct appbase *ab,
		const char *data,
		uint32_t length);
void appbase_close(struct appbase *);

void appbase_enable_progress(struct appbase *appbase, uint8_t enable);
void appbase_enable_verbose(struct appbase *appbase, uint8_t enable);

#endif /* APPBASE_H_ */
