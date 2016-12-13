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
 * File:   frequent_words.c
 * 
 * Content: Functions related to Step1 Frequent words.
 *
 * Created on November 30, 2016, 3:12 AM
 */

#include "common_header.h"
#include "frequent_words.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "output.h"
#include "line_processing.h"
#include "utility.h"
#include "word_filter_search_replace.h"
#include "hash_table_processing.h"

static tableindex_t create_word_sketch(struct Parameters *pParam);
static tableindex_t create_word_sketch_with_wfilter(struct Parameters *pParam);
static wordnumber_t create_vocabulary(struct Parameters *pParam);
static wordnumber_t create_vocabulary_with_wfilter(struct Parameters *pParam);

void step_1_create_word_sketch(struct Parameters *pParam)
{
  tableindex_t effect;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  log_msg("Creating the word sketch...", LOG_NOTICE, pParam);
  pParam->pWordSketch = (unsigned long *) malloc(sizeof(unsigned long) *
                           pParam->wordSketchSize);
  if (!pParam->pWordSketch)
  {
    log_msg(MALLOC_ERR_6014, LOG_ERR, pParam);
    exit(1);
  }
  
  if (!pParam->pWordFilter)
  {
    effect = create_word_sketch(pParam);
  }
  else
  {
    effect = create_word_sketch_with_wfilter(pParam);
  }
  
  
  str_format_int_grouped(digit, effect);
  sprintf(logStr, "%s slots in the word sketch >= support threshhold", digit);
  log_msg(logStr, LOG_INFO, pParam);
}

wordnumber_t step_1_create_vocabulary(struct Parameters *pParam)
{
  wordnumber_t totalWordNum;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  log_msg("Creating vocabulary...", LOG_NOTICE, pParam);
  pParam->ppWordTable = (struct Elem **) malloc(sizeof(struct Elem *) *
                          pParam->wordTableSize);
  if (!pParam->ppWordTable)
  {
    log_msg(MALLOC_ERR_6015, LOG_ERR, pParam);
    exit(1);
  }
  
  if (!pParam->pWordFilter)
  {
    totalWordNum = create_vocabulary(pParam);
  }
  else
  {
    totalWordNum = create_vocabulary_with_wfilter(pParam);
  }
  
  str_format_int_grouped(digit, totalWordNum);
  sprintf(logStr, "%s words were inserted into the vocabulary.", digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  return totalWordNum;
}

wordnumber_t step_1_find_frequent_words(struct Parameters *pParam, 
        wordnumber_t sum)
{
  tableindex_t i;
  wordnumber_t freWordNum;
  struct Elem *ptr, *pPrev, *pNext;
  struct WordFreqStat stat;
  char logStr[MAXLOGMSGLEN];
  float pct;
  char digit[MAXDIGITBIT];
  
  freWordNum = 0;
  
  stat.ones = 0;
  stat.twos = 0;
  stat.fives = 0;
  stat.tens = 0;
  stat.twenties = 0;
  
  for (i = 0; i < pParam->wordTableSize; i++)
  {
    if (!pParam->ppWordTable[i])
    {
      continue;
    }
    
    pPrev = 0;
    ptr = pParam->ppWordTable[i];
    
    while (ptr)
    {
      if (ptr->count == 1)  { stat.ones++; }
      if (ptr->count <= 2)  { stat.twos++; }
      if (ptr->count <= 5)  { stat.fives++; }
      if (ptr->count <= 10) { stat.tens++; }
      if (ptr->count <= 20) { stat.twenties++; }
      
      if (ptr->count < pParam->support)
      {
        if (pPrev)
        {
          pPrev->pNext = ptr->pNext;
        }
        else
        {
          pParam->ppWordTable[i] = ptr->pNext;
        }
        
        pNext = ptr->pNext;
        
        free((void *) ptr->pKey);
        free((void *) ptr);
        
        ptr = pNext;
      }
      else
      {
        /* Every frequent word gets a unique sequential ID, beginning
         from 1, ending at FreWordNum. This unique ID will be used in word
         dependency calculation. */
        ptr->number = ++freWordNum;
        pPrev = ptr;
        ptr = ptr->pNext;
      }
    }
  }
  
  str_format_int_grouped(digit, freWordNum);
  sprintf(logStr, "%s frequent words were found.", digit);
  log_msg(logStr, LOG_NOTICE, pParam);
  
  if (!freWordNum)
  {
    return 0;
  }
  
  str_format_int_grouped(digit, stat.ones);
  pct = ((float) stat.ones) / sum;
  sprintf(logStr, "%d%% - %s words in vocabulary occur 1 time.",
      (int) (pct * 100), digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, stat.twos);
  pct = ((float) stat.twos) / sum;
  sprintf(logStr, "%d%% - %s words in vocabulary occur 2 times or less.",
      (int) (pct * 100), digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, stat.fives);
  pct = ((float) stat.fives) / sum;
  sprintf(logStr, "%d%% - %s words in vocabulary occur 5 times or less.",
      (int) (pct * 100), digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, stat.tens);
  pct = ((float) stat.tens) / sum;
  sprintf(logStr, "%d%% - %s words in vocabulary occur 10 times or less.",
      (int) (pct * 100), digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, stat.twenties);
  pct = ((float) stat.twenties) / sum;
  sprintf(logStr, "%d%% - %s words in vocabulary occur 20 times or less.",
      (int) (pct * 100), digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, sum - freWordNum);
  pct = ((float) (sum - freWordNum)) / sum;
  sprintf(logStr, "%.2f%% - %s words in vocabulary occur less than "
      "%lu(support) times.", pct * 100, digit,
      (unsigned long) pParam->support);
  log_msg(logStr, LOG_INFO, pParam);
  
  return freWordNum;
}

/* The debug result is sorted, according to support in a descending order. */
void debug_1_print_frequent_words(struct Parameters *pParam)
{
  struct Elem **ppSortedArray;
  int i, j;
  struct Elem *ptr;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) *
                      pParam->freWordNum);
  if (!ppSortedArray)
  {
    log_msg(MALLOC_ERR_6012, LOG_ERR, pParam);
    exit(1);
  }
  
  j = 0;
  
  for (i = 0; i < pParam->wordTableSize; i++)
  {
    if (!pParam->ppWordTable[i])
    {
      continue;
    }
    
    ptr = pParam->ppWordTable[i];
    
    while (ptr)
    {
      ppSortedArray[j] = ptr;
      j++;
      ptr = ptr->pNext;
    }
  }
  
  sort_elements(ppSortedArray, pParam->freWordNum, pParam);
  
  for (i = 0; i < pParam->freWordNum; i++)
  {
    str_format_int_grouped(digit, ppSortedArray[i]->count);
    sprintf(logStr, "Frequent word: %s -- occurs in %s lines",
        ppSortedArray[i]->pKey, digit);
    log_msg(logStr, LOG_DEBUG, pParam);
  }
  
  free((void *) ppSortedArray);
}

/* When making changes to this function, don't forget to also change its
 brother function create_word_sketch_with_wfilter(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static tableindex_t create_word_sketch(struct Parameters *pParam)
{
  FILE *pFile;
  tableindex_t hash, j, oversupport;
  int len, i, wordcount;
  support_t linecount;
  struct InputFile *pFilePtr;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  
  linecount = 0;
  
  for (j = 0; j < pParam->wordSketchSize; j++)
  {
    pParam->pWordSketch[j] = 0;
  }
  
  for (pFilePtr = pParam->pInputFiles; pFilePtr; pFilePtr = pFilePtr->pNext)
  {
    if (!(pFile = fopen(pFilePtr->pName, "r")))
    {
      sprintf(logStr, "Can't open input file %s", pFilePtr->pName);
      log_msg(logStr, LOG_ERR, pParam);
      continue;
    }
    
    while (fgets(line, MAXLINELEN, pFile))
    {
      len = (int) strlen(line);
      if (line[len - 1] == '\n')
      {
        line[len - 1] = 0;
      }
      
      wordcount = find_words(line, words, pParam);
      
      for (i = 0; i < wordcount; i++)
      {
        if (words[i][0] == 0)
        {
          continue;
        }
        
        hash = str2hash(words[i], pParam->wordSketchSize,
                pParam->wordSketchSeed);
        
        pParam->pWordSketch[hash]++;
      }
      
      linecount++;
    }
    
    fclose(pFile);
  }
  
  if (!pParam->linecount)
  {
    pParam->linecount = linecount;
  }
  
  if (!pParam->support)
  {
    pParam->support = linecount * pParam->pctSupport / 100;
  }
  
  oversupport = 0;
  
  for (j = 0; j < pParam->wordSketchSize; j++)
  {
    if (pParam->pWordSketch[j] >= pParam->support)
    {
      oversupport++;
    }
  }
  
  return oversupport;
}

/* This is a redundant function, which works similarly as function
 create_word_sketch(), but with consideration of '--wfilter' option. Since
 this function has a coding style with overlapping IF-s, it could be read while
 comparing function create_word_sketch() to see the differences. */

/* This redundant function can be integrated into its original function.
 However, for the sake of performance and readability of the original function,
 this function is separated as an alone function. */

/* In program, if '--wfilter' option is not used, this function will not be
 called. */

/* When making changes to this function, don't forget to also change its
 brother function create_word_sketch(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static tableindex_t create_word_sketch_with_wfilter(struct Parameters *pParam)
{
  FILE *pFile;
  tableindex_t hash, j, oversupport;
  int len, i, wordcount;
  support_t linecount;
  struct InputFile *pFilePtr;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  
  
  linecount = 0;
  
  for (j = 0; j < pParam->wordSketchSize; j++)
  {
    pParam->pWordSketch[j] = 0;
  }
  
  for (pFilePtr = pParam->pInputFiles; pFilePtr; pFilePtr = pFilePtr->pNext)
  {
    if (!(pFile = fopen(pFilePtr->pName, "r")))
    {
      sprintf(logStr, "Can't open input file %s", pFilePtr->pName);
      log_msg(logStr, LOG_ERR, pParam);
      continue;
    }
    
    while (fgets(line, MAXLINELEN, pFile))
    {
      len = (int) strlen(line);
      if (line[len - 1] == '\n')
      {
        line[len - 1] = 0;
      }
      
      wordcount = find_words(line, words, pParam);
      
      for (i = 0; i < wordcount; i++)
      {
        if (words[i][0] == 0)
        {
          continue;
        }
        
        hash = str2hash(words[i], pParam->wordSketchSize,
                pParam->wordSketchSeed);
        
        pParam->pWordSketch[hash]++;
        
        if (is_word_filtered(words[i], pParam))
        {
          hash = str2hash(word_search_replace(words[i], pParam),
                  pParam->wordSketchSize,
                  pParam->wordSketchSeed);
          
          pParam->pWordSketch[hash]++;
        }
      }
      
      linecount++;
    }
    
    fclose(pFile);
  }
  
  if (!pParam->linecount)
  {
    pParam->linecount = linecount;
  }
  
  if (!pParam->support)
  {
    pParam->support = linecount * pParam->pctSupport / 100;
  }
  
  oversupport = 0;
  
  for (j = 0; j < pParam->wordSketchSize; j++)
  {
    if (pParam->pWordSketch[j] >= pParam->support)
    {
      oversupport++;
    }
  }
  
  return oversupport;
}

/* When making changes to this function, don't forget to also change its
 brother function create_vocabulary_with_wfilter(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_vocabulary(struct Parameters *pParam)
{
  wordnumber_t number = 0;
  tableindex_t j, hash;
  struct InputFile *pFilePtr;
  FILE *pFile;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  int i, len, wordcount, distinctWords;
  struct Elem *word;
  support_t linecount;
  
  
  for (j = 0; j < pParam->wordTableSize; j++)
  {
    pParam->ppWordTable[j] = 0;
  }
  
  for (pFilePtr = pParam->pInputFiles; pFilePtr; pFilePtr = pFilePtr->pNext)
  {
    if (!(pFile = fopen(pFilePtr->pName, "r")))
    {
      sprintf(logStr, "Can't open input file %s", pFilePtr->pName);
      log_msg(logStr, LOG_ERR, pParam);
      continue;
    }
    
    while (fgets(line, MAXLINELEN, pFile))
    {
      len = (int) strlen(line);
      if (line[len - 1] == '\n')
      {
        line[len - 1] = 0;
      }
      
      wordcount = find_words(line, words, pParam);
      
      distinctWords = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        if (words[i][0] == 0)
        {
          continue;
        }
        
        /* The technique to save memory space. */
        if (pParam->wordSketchSize)
        {
          hash = str2hash(words[i], pParam->wordSketchSize, 
                  pParam->wordSketchSeed);
          if (pParam->pWordSketch[hash] < pParam->support)
          {
            continue;
          }
        }
        
        word = add_elem(words[i], pParam->ppWordTable, pParam->wordTableSize, 
                pParam->wordTableSeed, pParam);
        distinctWords++;
        
        if (word->count == 1)
        {
          number++;
          word->number = number;
        }
        
        /* If word is repeated..its support will not increment more than
         once in one log line. */
        if (is_word_repeated(pParam->wordNumStr, word->number,
                   distinctWords))
        {
          distinctWords--;
          word->count--;
        }
        else
        {
          pParam->wordNumStr[distinctWords] = word->number;
        }
        
      }
      
      linecount++;
    }
    
    fclose(pFile);
    
  }
  
  if (!pParam->linecount)
  {
    pParam->linecount = linecount;
  }
  
  if (!pParam->support)
  {
    pParam->support = linecount * pParam->pctSupport / 100;
  }
  
  return number;
}

/* This is a redundant function, which works similarly as function
 create_vocabulary(), but with consideration of '--wfilter' option. Since
 this function has a coding style with overlapping IF-s, it could be read while
 comparing function create_vocabulary() to see the differences. */

/* This redundant function can be integrated into its original function.
 However, for the sake of performance and readability of the original function,
 this function is separated as an alone function. */

/* In program, if '--wfilter' option is not used, this function will not be
 called. */

/* When making changes to this function, don't forget to also change its
 brother function create_vocabulary(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_vocabulary_with_wfilter(struct Parameters *pParam)
{
  wordnumber_t number = 0;
  tableindex_t j, hash;
  struct InputFile *pFilePtr;
  FILE *pFile;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];   /*10240*/
  char words[MAXWORDS][MAXWORDLEN];   /*512 10248*/
  int i, len, wordcount, distinctWords;
  struct Elem *word;
  support_t linecount;
  char newWord[MAXWORDLEN];
  
  *newWord = 0;
  
  for (j = 0; j < pParam->wordTableSize; j++)
  {
    pParam->ppWordTable[j] = 0;
  }
  
  for (pFilePtr = pParam->pInputFiles; pFilePtr; pFilePtr = pFilePtr->pNext)
  {
    if (!(pFile = fopen(pFilePtr->pName, "r")))
    {
      sprintf(logStr, "Can't open input file %s", pFilePtr->pName);
      log_msg(logStr, LOG_ERR, pParam);
      continue;
    }
    
    while (fgets(line, MAXLINELEN, pFile))
    {
      len = (int) strlen(line);
      if (line[len - 1] == '\n')
      {
        line[len - 1] = 0;
      }
      
      wordcount = find_words(line, words, pParam);
      
      distinctWords = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        if (words[i][0] == 0)
        {
          continue;
        }
        
        if (pParam->wordSketchSize)
        {
          hash = str2hash(words[i], pParam->wordSketchSize,
                  pParam->wordSketchSeed);
          
          if (pParam->pWordSketch[hash] >= pParam->support)
          {
            word = add_elem(words[i], pParam->ppWordTable,
                    pParam->wordTableSize,
                    pParam->wordTableSeed,
                    pParam);
            
            distinctWords++;
            
            if (word->count == 1)
            {
              number++;
              word->number = number;
            }
            
            /* If word is repeated..its support will not increment
             more than once in one log line. */
            if (is_word_repeated(pParam->wordNumStr, word->number,
                       distinctWords))
            {
              distinctWords--;
              word->count--;
            }
            else
            {
              pParam->wordNumStr[distinctWords] = word->number;
            }
            
          }
          
          if (is_word_filtered(words[i], pParam))
          {
            strcpy(newWord, word_search_replace(words[i], pParam));
            hash = str2hash(newWord, pParam->wordSketchSize,
                    pParam->wordSketchSeed);
            
            if (pParam->pWordSketch[hash] >= pParam->support)
            {
              word = add_elem(newWord, pParam->ppWordTable,
                      pParam->wordTableSize,
                      pParam->wordTableSeed,
                      pParam);
              
              distinctWords++;
              
              if (word->count == 1)
              {
                number++;
                word->number = number;
              }
              
              /* If word is repeated..its support will not
               increment more than once in one log line. */
              if (is_word_repeated(pParam->wordNumStr,
                         word->number, distinctWords))
              {
                distinctWords--;
                word->count--;
              }
              else
              {
                pParam->wordNumStr[distinctWords]= word->number;
              }
              
            }
          }
        }
        else
        {
          word = add_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize,
                  pParam->wordTableSeed,
                  pParam);
          
          distinctWords++;
          
          if (word->count == 1)
          {
            number++;
            word->number = number;
          }
          
          /* If word is repeated..its support will not increment more
           than once in one log line. */
          if (is_word_repeated(pParam->wordNumStr, word->number,
                     distinctWords))
          {
            distinctWords--;
            word->count--;
          }
          else
          {
            pParam->wordNumStr[distinctWords] = word->number;
          }
          
          if (is_word_filtered(words[i], pParam))
          {
            strcpy(newWord, word_search_replace(words[i], pParam));
            word = add_elem(newWord, pParam->ppWordTable,
                    pParam->wordTableSize,
                    pParam->wordTableSeed,
                    pParam);
            
            distinctWords++;
            
            if (word->count == 1)
            {
              number++;
              word->number = number;
            }
            
            /* If word is repeated..its support will not increment
             more than once in one log line. */
            if (is_word_repeated(pParam->wordNumStr, word->number,
                       distinctWords))
            {
              distinctWords--;
              word->count--;
            }
            else
            {
              pParam->wordNumStr[distinctWords] = word->number;
            }
          }
        }
      }
      
      linecount++;
    }
    
    fclose(pFile);
    
  }
  
  if (!pParam->linecount)
  {
    pParam->linecount = linecount;
  }
  
  if (!pParam->support)
  {
    pParam->support = linecount * pParam->pctSupport / 100;
  }
  
  return number;
}

