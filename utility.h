/*
 * Copyright (C) 2016 Zhuge Chen, Risto Vaarandi and Mauno Pihelgas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* 
 * File:   utility.h
 * 
 * Content: Declarations of global functions in utility.c .
 *
 * Created on November 29, 2016, 11:12 PM
 */

#ifndef UTILITY_H
#define UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
  
#include "macro.h"
  
void string_lowercase(char *p);
size_t str_format_int_grouped(char dst[MAXDIGITBIT], unsigned long num);
tableindex_t str2hash(char *string, tableindex_t modulo, tableindex_t h);
void sort_elements(struct Elem **ppArray, wordnumber_t size,
           struct Parameters *pParam);
void gen_random_string(char *s, const int len);

#ifdef __cplusplus
}
#endif

#endif /* UTILITY_H */

