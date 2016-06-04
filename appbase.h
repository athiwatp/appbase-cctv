/*
 * appbase.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef APPBASE_H_
#define APPBASE_H_

struct appbase;

struct appbase *appbase_open(const char *app_name,
		const char *username,
		const char *password);
int appbase_push_frame(struct appbase *ab,
		const char *data,
		unsigned int length);
void appbase_close(struct appbase *appbase);

void appbase_enable_progress(struct appbase *appbase, int enable);
void appbase_enable_verbose(struct appbase *appbase, int enable);

#endif /* APPBASE_H_ */
