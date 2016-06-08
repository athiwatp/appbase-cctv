/*
 * utils.h
 *
 *  Created on: 31 May 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <stddef.h>

/* Preprocessor fun - Define type bool :) */
#ifdef __cplusplus
#  define _Bool bool
#  define bool bool
#else
#  if !defined __GNUC__
#    define _Bool signed char
#  else
#    if !HAVE__BOOL
typedef enum {
	_Bool_must_promote_to_int = -1,
	false = 0,
	true = 1
} Bool;
#define _Bool Bool
#    endif /* !@HAVE_BOOL@ */
#  endif /* __GNUC__ */
#  define bool _Bool
#endif /* __cplusplus */

#ifdef __cplusplus
#  define true true
#  define false false
#else
#  define true 1
#  define false 0
#endif

/* This typically runs as a daemon. Tweak this as desired. */
#define SYSLOG_FACILITY 	LOG_DAEMON

void fatal(const char *message);
void *ec_malloc(size_t size);

#endif /* UTILS_H_ */
