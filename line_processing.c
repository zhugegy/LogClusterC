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
 * File:   line_processing.c
 * 
 * Content: Functions related to line processing. Line processing is a primary 
 * task of the program.
 *
 * Created on November 30, 2016, 3:32 AM
 */

#include "common_header.h"
#include "line_processing.h"

#include <regex.h>     /* for regcomp() and regexec() */
#include <time.h>      /* for time() and ctime() */
#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "utility.h"
#include "output.h"

static int find_words_debug_0_1(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam);
static int find_words_debug_2(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam);
static int find_words_debug_3(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam);

/* The three sub functions can be integrated into one function. However, for
 the sake of performance and code readability, they are divided. When making
 changes to one function, don't forget to also change the corresponding lines
 in the other two functions. Sorry for this inconvenient design. It will be
 fixed in the following updates (considering to integrate all these 
 possibilities, regardless of readability. Or some other better upcoming 
 solutions). */
/* The words in one log line will be copied to char (*words)[MAXWORDLEN] for 
 later process. Returns the number of words in one log line. */
int find_words(char *line, char (*words)[MAXWORDLEN], struct Parameters *pParam)
{
  switch (pParam->debug)
  {
    case 0:
    case 1:
      return find_words_debug_0_1(line, words, pParam);
      break;
    case 2:
      return find_words_debug_2(line, words, pParam);
      break;
    case 3:
      return find_words_debug_3(line, words, pParam);
      break;
    default:
      break;
  }
  
  return 0;
}

int is_word_repeated(wordnumber_t *storage, wordnumber_t wordNumber, int serial)
{
  int i;
  
  for (i = 1; i < serial; i++)
  {
    if (storage[i] == wordNumber)
    {
      return 1;
    }
  }
  
  return 0;
}

/* When making changes to this function, don't forget to also change the 
 corresponding lines in the other two brother functions. They are:
 find_words_debug_0_1(), find_words_debug_2(), find_words_debug_3(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static int find_words_debug_0_1(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam)
{
  regmatch_t match[MAXPARANEXPR];
  
  int i, j, linelen, len;
  struct TemplElem *ptr;
  char *buffer = NULL;
  
  if (*line == 0)
  {
    return 0;
  }
  
  linelen = (int) strlen(line);
  
  
  if (pParam->byteOffset >= linelen)  { return 0; }
  
  if (pParam->byteOffset)
  {
    line += pParam->byteOffset;
    linelen -= pParam->byteOffset;
  }
  
  if (pParam->pFilter)
  {
    if (regexec(&pParam->filter_regex, line, MAXPARANEXPR, match, 0))
    {
      return 0;
    }
    
    if (pParam->pTemplate)
    {
      
      len = 0;
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        if (ptr->pStr)
        {
          len += ptr->data;
        }
        else if (!ptr->data)
        {
          len += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len += match[ptr->data].rm_eo - match[ptr->data].rm_so;
        }
      }
      
      i = 0;
      //free((void *) buffer);
      buffer = (char *) malloc(len + 1);
      if (!buffer)
      {
        log_msg(MALLOC_ERR_6008, LOG_ERR, pParam);
        exit(1);
      }
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        
        if (ptr->pStr)
        {
          strncpy(buffer + i, ptr->pStr, ptr->data);
          i += ptr->data;
        }
        else if (!ptr->data)
        {
          strncpy(buffer + i, line, linelen);
          i += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len = (int) (match[ptr->data].rm_eo -
                 match[ptr->data].rm_so);
          strncpy(buffer + i, line + match[ptr->data].rm_so, len);
          i += len;
        }
      }
      
      buffer[i] = 0;
      line = buffer;
    }
    
  }
  
  for (i = 0; i < MAXWORDS; ++i)
  {
    if (regexec(&pParam->delim_regex, line, 1, match, 0))
    {  /* This is the last word. */
      for (j = 0; line[j] != 0; j++)
      {
        words[i][j] = line[j];
      }
      words[i][j] = 0;
      
      break;
    }
    
    for (j = 0; j < match[0].rm_so; ++j)
    {
      words[i][j] = line[j];
    }
    
    words[i][j] = 0;
    
    line += match[0].rm_eo;
    
    if (*line == 0)
    {
      break;
    }
  }
  
  if (pParam->pTemplate)
  {
    free((void *) buffer);
  }
  
  /* Return the word numbers in the line, including the repeated ones. */
  if (i == MAXWORDS)
  {
    return i;
  }
  else
  {
    return i+1;
  }
}

/* When making changes to this function, don't forget to also change the 
 corresponding lines in the other two brother functions. They are:
 find_words_debug_0_1(), find_words_debug_2(), find_words_debug_3(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static int find_words_debug_2(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam)
{
  regmatch_t match[MAXPARANEXPR];
  
  int i, j, linelen, len;
  struct TemplElem *ptr;
  char *buffer = NULL;
  
  //debug2
  static support_t linecnt = 0;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  double pct;
  
  if (*line == 0)
  {
    return 0;
  }
  
  linelen = (int) strlen(line);
  
  
  if (pParam->byteOffset >= linelen)  { return 0; }
  
  if (pParam->byteOffset)
  {
    line += pParam->byteOffset;
    linelen -= pParam->byteOffset;
  }
  
  if (pParam->pFilter)
  {
    if (regexec(&pParam->filter_regex, line, MAXPARANEXPR, match, 0))
    {
      return 0;
    }
    
    if (pParam->pTemplate)
    {
      
      len = 0;
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        if (ptr->pStr)
        {
          len += ptr->data;
        }
        else if (!ptr->data)
        {
          len += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len += match[ptr->data].rm_eo - match[ptr->data].rm_so;
        }
      }
      
      i = 0;
      //free((void *) buffer);
      buffer = (char *) malloc(len + 1);
      if (!buffer)
      {
        log_msg(MALLOC_ERR_6008, LOG_ERR, pParam);
        exit(1);
      }
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        
        if (ptr->pStr)
        {
          strncpy(buffer + i, ptr->pStr, ptr->data);
          i += ptr->data;
        }
        else if (!ptr->data)
        {
          strncpy(buffer + i, line, linelen);
          i += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len = (int) (match[ptr->data].rm_eo -
                 match[ptr->data].rm_so);
          strncpy(buffer + i, line + match[ptr->data].rm_so, len);
          i += len;
        }
      }
      
      buffer[i] = 0;
      line = buffer;
    }
    
  }
  
  for (i = 0; i < MAXWORDS; ++i)
  {
    if (regexec(&pParam->delim_regex, line, 1, match, 0))
    {  /* This is the last word. */
      for (j = 0; line[j] != 0; j++)
      {
        words[i][j] = line[j];
      }
      words[i][j] = 0;
      
      break;
    }
    
    for (j = 0; j < match[0].rm_so; ++j)
    {
      words[i][j] = line[j];
    }
    
    words[i][j] = 0;
    
    line += match[0].rm_eo;
    
    if (*line == 0)
    {
      break;
    }
  }
  
  if (pParam->pTemplate)
  {
    free((void *) buffer);
  }
  
  //debug_2
  linecnt++;
  if (linecnt % DEBUG_2_INTERVAL == 0)
  {
    str_format_int_grouped(digit, linecnt);
    if (pParam->totalLineNum)
    {
      pct = (double) linecnt / pParam->totalLineNum;
      sprintf(logStr, "%.2f%% Finished. - %s lines out of %s", pct * 100,
          digit, pParam->totalLineNumDigit);
    }
    else
    {
      sprintf(logStr, "UNKNOWN%% Finished. - %s lines out of UNKNOWN.",
          digit);
    }
    
    log_msg(logStr, LOG_DEBUG, pParam);
  }
  
  /* Return the word numbers in the line, including the repeated ones. */
  if (i == MAXWORDS)
  {
    return i;
  }
  else
  {
    return i+1;
  }
}

/* When making changes to this function, don't forget to also change the 
 corresponding lines in the other two brother functions. They are:
 find_words_debug_0_1(), find_words_debug_2(), find_words_debug_3(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static int find_words_debug_3(char *line, char (*words)[MAXWORDLEN],
             struct Parameters *pParam)
{
  regmatch_t match[MAXPARANEXPR];
  
  int i, j, linelen, len;
  struct TemplElem *ptr;
  char *buffer = NULL;
  
  //debug3
  static support_t linecnt = 0;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  double pct;
  
  if (*line == 0)
  {
    return 0;
  }
  
  linelen = (int) strlen(line);
  
  
  if (pParam->byteOffset >= linelen)  { return 0; }
  
  if (pParam->byteOffset)
  {
    line += pParam->byteOffset;
    linelen -= pParam->byteOffset;
  }
  
  if (pParam->pFilter)
  {
    if (regexec(&pParam->filter_regex, line, MAXPARANEXPR, match, 0))
    {
      return 0;
    }
    
    if (pParam->pTemplate)
    {
      
      len = 0;
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        if (ptr->pStr)
        {
          len += ptr->data;
        }
        else if (!ptr->data)
        {
          len += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len += match[ptr->data].rm_eo - match[ptr->data].rm_so;
        }
      }
      
      i = 0;
      //free((void *) buffer);
      buffer = (char *) malloc(len + 1);
      if (!buffer)
      {
        log_msg(MALLOC_ERR_6008, LOG_ERR, pParam);
        exit(1);
      }
      
      for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
      {
        
        if (ptr->pStr)
        {
          strncpy(buffer + i, ptr->pStr, ptr->data);
          i += ptr->data;
        }
        else if (!ptr->data)
        {
          strncpy(buffer + i, line, linelen);
          i += linelen;
        }
        else if (match[ptr->data].rm_so != -1  &&
             match[ptr->data].rm_eo != -1)
        {
          len = (int) (match[ptr->data].rm_eo -
                 match[ptr->data].rm_so);
          strncpy(buffer + i, line + match[ptr->data].rm_so, len);
          i += len;
        }
      }
      
      buffer[i] = 0;
      line = buffer;
    }
    
  }
  
  for (i = 0; i < MAXWORDS; ++i)
  {
    if (regexec(&pParam->delim_regex, line, 1, match, 0))
    {  /* This is the last word. */
      for (j = 0; line[j] != 0; j++)
      {
        words[i][j] = line[j];
      }
      words[i][j] = 0;
      
      break;
    }
    
    for (j = 0; j < match[0].rm_so; ++j)
    {
      words[i][j] = line[j];
    }
    
    words[i][j] = 0;
    
    line += match[0].rm_eo;
    
    if (*line == 0)
    {
      break;
    }
  }
  
  if (pParam->pTemplate)
  {
    free((void *) buffer);
  }
  
  //debug_3
  linecnt++;
  if (time(0) != pParam->timeStorage && time(0) % DEBUG_3_INTERVAL == 0)
  {
    pParam->timeStorage = time(0);
    str_format_int_grouped(digit, linecnt);
    if (pParam->totalLineNum)
    {
      pct = (double) linecnt / pParam->totalLineNum;
      sprintf(logStr, "%.2f%% Finished. - %s lines out of %s", pct * 100,
          digit, pParam->totalLineNumDigit);
    }
    else
    {
      sprintf(logStr, "UNKNOWN%% Finished. - %s lines out of UNKNOWN.",
          digit);
    }
    
    log_msg(logStr, LOG_DEBUG, pParam);
  }
  
  /* Return the word numbers in the line, including the repeated ones. */
  if (i == MAXWORDS)
  {
    return i;
  }
  else
  {
    return i+1;
  }
}