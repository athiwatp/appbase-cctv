/*
 * cb.c
 *
 * A circular buffer implementation of data buckets
 * using POSIX semaphores. It is designed to be used
 * by only one writer and one reader processes.
 *
 *  Created on: 4 Jul 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <semaphore.h>
#include "cb.h"
#include "utils.h"

struct bucket {
	char *data;
	size_t len;
};

struct cb {
	uint8_t read;
	uint8_t write;
	uint8_t buflen;
	sem_t sem;
	struct bucket *buf;
};

bool cb_try_next(struct cb *cb, const char **data, size_t *len)
{
	int idx;
	bool retval = false;

	if (!cb || !data || !len)
		return false;

	idx = cb->read % cb->buflen;

	if (sem_trywait(&cb->sem) == 0) {
		*data = cb->buf[idx].data;
		*len = cb->buf[idx].len;

		cb->read++;
		retval = true;
	}

	return retval;
}

void cb_append(struct cb *cb, const char *data, size_t len)
{
	int idx;

	if (!cb)
		return;

	idx = cb->write % cb->buflen;

	cb->buf[idx].data = (char *) data;
	cb->buf[idx].len = len;
	sem_post(&cb->sem);

	cb->write++;
}

struct cb *cb_start(uint8_t buflen)
{
	struct cb *cb = ec_malloc(sizeof(struct cb));

	if (!buflen)
		goto fail;

	cb->buflen = buflen;
	cb->read = 0;
	cb->write = 0;
	cb->buf = ec_malloc(sizeof(struct bucket) * buflen);

	sem_init(&cb->sem, 0, 0);

	return cb;

fail:
	free(cb);
	return NULL;
}

void cb_destroy(struct cb *cb)
{
	if (cb) {
		if (cb->buf)
			free(cb->buf);
		sem_destroy(&cb->sem);
		free(cb);
	}
}
