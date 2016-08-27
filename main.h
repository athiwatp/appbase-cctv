/*
 * main.h
 *
 *  Created on: 23 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */

#ifndef MAIN_H_
#define MAIN_H_

/* Preprocessor fun - Define type bool :) */
#ifdef __cplusplus
#  define _Bool bool
#  define bool bool
#else
#  if !defined __GNUC__
#    define _Bool signed char
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

#endif /* MAIN_H_ */
