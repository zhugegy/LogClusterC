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
 * File:   utility.c
 * 
 * Content: Useful utility functions.
 *
 * Created on November 29, 2016, 11:11 PM
 */

#include "common_header.h"
#include "utility.h"

#include <ctype.h>     /* for tolower() */



/* String lower case convertion, by by J.F. Sebastian. */
void string_lowercase(char *p)
{
  for ( ; *p; ++p) *p = tolower(*p);
}

/* Insert commas into numbers, between every three digits. */
/* Based on @Greg Hewgill's, from ideasman42. */
/* http://stackoverflow.com/questions/1449805/how-to-format-a-number-from-1123456789-to-1-123-456-789-in-c */
size_t str_format_int_grouped(char dst[MAXDIGITBIT], unsigned long num)
{
  char src[MAXDIGITBIT];
  char *p_src = src;
  char *p_dst = dst;
  
  const char separator = ',';
  int num_len, commas;
  
  num_len = sprintf(src, "%lu", num);
  
  if (*p_src == '-')
  {
    *p_dst++ = *p_src++;
    num_len--;
  }
  
  for (commas = 2 - num_len % 3; *p_src; commas = (commas + 1) % 3)
  {
    *p_dst++ = *p_src++;
    if (commas == 1)
    {
      *p_dst++ = separator;
    }
  }
  *--p_dst = '\0';
  
  return (size_t)(p_dst - dst);
}

/* Fast string hashing algorithm by M.V.Ramakrishna and Justin Zobel. */
tableindex_t str2hash(char *string, tableindex_t modulo, tableindex_t h)
{
  int i;
  for (i = 0; string[i] != 0; ++i)
  {
    h = h ^ ((h << 5) + (h >> 2) + string[i]);
  }
  return h % modulo;
}

/* Sort according to support value. */
void sort_elements(struct Elem **ppArray, wordnumber_t size,
           struct Parameters *pParam)
{
  int i, j, imax;
  struct Elem *tmp;
  
  for (j = 0; j < size - 1; j++)
  {
    imax = j;
    
    for (i = j + 1; i < size; i++)
    {
      if (ppArray[i]->count > ppArray[imax]->count)
      {
        imax = i;
      }
    }
    tmp = ppArray[j];
    ppArray[j] = ppArray[imax];
    ppArray[imax] = tmp;
  }
}

/* Ates Goral's solution for generating random string. Used for token
 generation. */
/* http://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c */
void gen_random_string(char *s, const int len)
{
  int i;
  static const char alphanum[] =
  "0123456789"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz";
  
  for (i = 0; i < len; ++i)
  {
    /* Maybe rand() % (sizeof(alphanum)) ? */
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  
  s[len] = 0;
}