/*
 * cb.h
 *
 * A circular buffer implementation of data buckets
 * using POSIX semaphores. It is designed to be used
 * by only one writer and one reader processes.
 *
 *  Created on: 4 Jul 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef CB_H_
#define CB_H_
#include <stdint.h>
#include "main.h"

struct cb;

struct cb *cb_start(uint8_t buflen);
void cb_destroy(struct cb *cb);

void cb_append(struct cb *, const char *, size_t);
bool cb_try_next(struct cb *, const char **, size_t *);

#endif /* CB_H_ */
