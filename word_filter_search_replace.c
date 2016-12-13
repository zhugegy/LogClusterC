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
 * File:   word_filter_search_replace.c
 * 
 * Content: Functions related to '--wfilter', '--wsearch' and '--wreplace' 
 * options.
 *
 * Created on November 30, 2016, 4:13 AM
 */

#include "common_header.h"
#include "word_filter_search_replace.h"

#include <regex.h>     /* for regcomp() and regexec() */
#include <string.h>

static int check_endless_loop(long long start, long long end, 
        struct Parameters *pParm);
static void replace_string_for_word_search(long long start, long long end,
                  char *pOriginStr, char *pStr);

/* Check if the word can be filtered and replaced with user specified string.
 The word should not only contain the regex in '--wfilter', but also contain
 the regex in '--wsearch' option. Otherwise, if it only satisfies '--wfilter',
 it will be counted twice when build the vocabulary. Then it will cause other
 sequentially problems. */
int is_word_filtered(char *pStr, struct Parameters *pParam)
{
  if (!regexec(&pParam->wfilter_regex, pStr, 0, 0, 0) &&
    !regexec(&pParam->wsearch_regex, pStr, 0, 0, 0))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

char *word_search_replace(char *pOriginStr, struct Parameters *pParam)
{
  regmatch_t match[MAXPARANEXPR];
  int cnt;
  
  strcpy(pParam->tmpStr, pOriginStr);
  cnt = 0;
  
  while (1)
  {
    if (!regexec(&pParam->wsearch_regex, pParam->tmpStr, 1, match, 0))
    {
      if (cnt && !check_endless_loop(match[0].rm_so, match[0].rm_eo,
                       pParam))
      {
        break;
      }
      replace_string_for_word_search(match[0].rm_so, match[0].rm_eo,
                       pParam->tmpStr, pParam->pWordReplace);
      cnt++;
    }
    else
    {
      break;
    }
  }
  
  return pParam->tmpStr;
}

/* Avoid endless loop. There will be endless loop if this function is not
 called, in the example below:
 --wfilter=’=’, --wsearch=’=.+’, and --wreplace=’=VALUE’
 After replacing matched string with user specified string '=VALUE', the regex
 is still true, thus it causes endless loop, keeping replacing '=VALUE' with
 '=VALUE'. */
static int check_endless_loop(long long start, long long end, 
        struct Parameters *pParm)
{
  long long i;
  int j;
  
  j = 0;
  for (i = start; i < end; i++)
  {
    if (pParm->tmpStr[i] != pParm->pWordReplace[j])
    {
      return 1;
    }
    else
    {
      j++;
    }
  }
  return 0;
}

/* Function dedicated to '--wfilter/--wsearch/--wreplace' options. */
static void replace_string_for_word_search(long long start, long long end,
                  char *pOriginStr, char *pStr)
{
  char tmp[MAXWORDLEN];
  int j, u, lenOriginStr, lenStr;
  long long i;
  
  *tmp = 0;
  j = 0;
  
  lenOriginStr = (int) strlen(pOriginStr);
  lenStr = (int) strlen(pStr);
  
  for (i = end; i < lenOriginStr; i++)
  {
    tmp[j] = pOriginStr[i];
    j++;
  }
  tmp[j] = 0;
  
  for (u = 0; u < lenStr; u++)
  {
    pOriginStr[start] = pStr[u];
    start++;
  }
  pOriginStr[start] = 0;
  
  strcat(pOriginStr, tmp);
}