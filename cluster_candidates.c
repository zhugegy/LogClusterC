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
 * File:   cluster_candidates.c
 * 
 * Content: Functions related to cluster candidate generation.
 *
 * Created on November 30, 2016, 6:10 AM
 */

#include "common_header.h"
#include "cluster_candidates.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "output.h"
#include "line_processing.h"
#include "hash_table_processing.h"
#include "utility.h"
#include "word_filter_search_replace.h"
#include "join_clusters_heuristic.h"

static tableindex_t create_cluster_candidate_sketch(struct Parameters *pParam);
static tableindex_t create_cluster_candidate_sketch_with_wfilter(
  struct Parameters *pParam);
static wordnumber_t create_cluster_candidates_word_dep(struct Parameters 
  *pParam);
static wordnumber_t create_cluster_candidates_word_dep_with_filter(
  struct Parameters *pParam);
static wordnumber_t create_cluster_candidates(struct Parameters *pParam);
static wordnumber_t create_cluster_candidates_with_wfilter(struct Parameters 
  *pParam);


static struct Cluster *create_cluster_instance(struct Elem* pClusterElem,
                    int constants, int wildcard[],
                    struct Elem *pStorage[],
                    struct Parameters *pParam);
static void adjust_cluster_instance(struct Elem* pClusterElem, int constants,
               int wildcard[], struct Parameters *pParam);

void step_2_create_cluster_candidate_sketch(struct Parameters *pParam)
{
  tableindex_t effect;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  log_msg("Creating the cluster sketch...", LOG_NOTICE, pParam);
  pParam->pClusterSketch = (unsigned long *)
  malloc(sizeof(unsigned long) * pParam->clusterSketchSize);
  if (!pParam->pClusterSketch)
  {
    log_msg(MALLOC_ERR_6016, LOG_ERR, pParam);
    exit(1);
  }
  
  if (!pParam->pWordFilter)
  {
    effect = create_cluster_candidate_sketch(pParam);
  }
  else
  {
    effect = create_cluster_candidate_sketch_with_wfilter(pParam);
  }
  
  str_format_int_grouped(digit, effect);
  sprintf(logStr, "%s slots in the cluster sketch >= support threshold.",
      digit);
  log_msg(logStr, LOG_INFO, pParam);
}

/* For the sake of computing speed, four brother functions (
 create_cluster_candidates(), create_cluster_candidates_with_wfilter(),
 create_cluster_candidates_word_dep(), and 
 create_cluster_candidates_word_dep_with_filter()) which have similar 
 function but with a few differences to each other are contained in this 
 function in parallel. This design of course brings inconvenience for future
 maintenance, and it will be fixed with better solution in following updates. */
void step_2_find_cluster_candidates(struct Parameters *pParam)
{
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  int i;
  
  log_msg("Finding cluster candidates...", LOG_NOTICE, pParam);
  if (!pParam->clusterTableSize)
  {
    pParam->clusterTableSize = 100 * pParam->freWordNum;
  }
  pParam->ppClusterTable = (struct Elem **) malloc(sizeof(struct Elem *) *
                           pParam->clusterTableSize);
  if (!pParam->ppClusterTable)
  {
    log_msg(MALLOC_ERR_6017, LOG_ERR, pParam);
    exit(1);
  }
  
  /* For option '--wweight'. For the sake of computing speed, the matrix 
   building process is integrated into this step (find_cluster_candidates). */
  if (pParam->wordWeightThreshold)
  {
    pParam->wordDepMatrixBreadth = pParam->freWordNum + 1;
    
    pParam->wordDepMatrix = (unsigned long *)
    malloc(sizeof(unsigned long) * pParam->wordDepMatrixBreadth *
         pParam->wordDepMatrixBreadth);
    if (!pParam->wordDepMatrix)
    {
      log_msg(MALLOC_ERR_6017, LOG_ERR, pParam);
      exit(1);
    }
    
    for (i = 0; i < pParam->wordDepMatrixBreadth *
       pParam->wordDepMatrixBreadth; i++)
    {
      pParam->wordDepMatrix[i] = 0;
    }
    
    if (!pParam->pWordFilter)
    {
      pParam->clusterCandiNum =
      create_cluster_candidates_word_dep(pParam);
    }
    else
    {
      pParam->clusterCandiNum =
      create_cluster_candidates_word_dep_with_filter(pParam);
    }
  }
  else
  {
    if (!pParam->pWordFilter)
    {
      pParam->clusterCandiNum = create_cluster_candidates(pParam);
    }
    else
    {
      pParam->clusterCandiNum =
      create_cluster_candidates_with_wfilter(pParam);
    }
    
  }
    
  str_format_int_grouped(digit, pParam->clusterCandiNum);
  sprintf(logStr, "%s cluster candidates were found.", digit);
  log_msg(logStr, LOG_INFO, pParam);
}

/* The debug result is sorted, according to support in a descending order. */
void debug_1_print_cluster_candidates(struct Parameters *pParam)
{
  struct Elem **ppSortedArray;
  int i, j;
  struct Elem *ptr;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) *
                      pParam->clusterCandiNum);
  if (!ppSortedArray)
  {
    log_msg(MALLOC_ERR_6013, LOG_ERR, pParam);
    exit(1);
  }
  
  j = 0;
  
  for (i = 0; i < pParam->clusterTableSize; i++)
  {
    if (!pParam->ppClusterTable[i])
    {
      continue;
    }
    
    ptr = pParam->ppClusterTable[i];
    
    while (ptr)
    {
      ppSortedArray[j] = ptr;
      j++;
      ptr = ptr->pNext;
    }
  }
  
  sort_elements(ppSortedArray, pParam->clusterCandiNum, pParam);
  
  for (i = 0; i < pParam->clusterCandiNum; i++)
  {
    str_format_int_grouped(digit, ppSortedArray[i]->count);
    print_cluster_to_string(ppSortedArray[i]->pCluster, pParam);
    sprintf(logStr, "Cluster candidate with support %s: %s", digit,
        pParam->clusterDescription);
    log_msg(logStr, LOG_DEBUG, pParam);
  }
  
  free((void *) ppSortedArray);
}

/* When making changes to this function, don't forget to also change its
 brother function create_cluster_candidate_sketch_with_wfilter(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static tableindex_t create_cluster_candidate_sketch(struct Parameters *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash, oversupport;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int len, wordcount, last, i;
  struct Elem *pWord;
  
  for (j = 0; j < pParam->clusterSketchSize; j++)
  {
    pParam->pClusterSketch[j] = 0;
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
      
      last = 0;
      *key = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          //last records the location of the last constant. */
          last = i + 1;
        }
      }
      
      if (!last)
      {
        /* !last means there is no frequent word in this line. */
        continue;
      }
      
      hash = str2hash(key, pParam->clusterSketchSize,
              pParam->clusterSketchSeed);
      pParam->pClusterSketch[hash]++;
    }
    
    fclose(pFile);
  }
  
  oversupport = 0;
  for (j = 0; j < pParam->clusterSketchSize; j++)
  {
    if (pParam->pClusterSketch[j] >= pParam->support)
    {
      oversupport++;
    }
  }
  
  return oversupport;
}

/* This is a redundant function, which works similarly as function
 create_cluster_candidate_sketch(), but with consideration of '--wfilter'
 option. Since this function has a coding style with overlapping IFs, it could
 be read while comparing function create_cluster_candidate_sketch() to see the
 differences. */

/* This redundant function can be integrated into its original function.
 However, for the sake of performance and readability of the original function,
 this function is separated as an alone function. */

/* In program, if '--wfilter' option is not used, this function will not be
 called. */

/* When making changes to this function, don't forget to also change its
 brother function create_cluster_candidate_sketch(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static tableindex_t create_cluster_candidate_sketch_with_wfilter(
  struct Parameters *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash, oversupport;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int len, wordcount, last, i;
  struct Elem *pWord;
  char newWord[MAXWORDLEN];
  
  *newWord = 0;
  
  for (j = 0; j < pParam->clusterSketchSize; j++)
  {
    pParam->pClusterSketch[j] = 0;
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
      
      last = 0;
      *key = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          /* last records the location of the last constant. */
          last = i + 1;
        }
        else if(is_word_filtered(words[i], pParam))
        {
          strcpy(newWord, word_search_replace(words[i], pParam));
          pWord = find_elem(newWord, pParam->ppWordTable,
                    pParam->wordTableSize,
                    pParam->wordTableSeed);
          if (words[i][0] != 0 && pWord)
          {
            strcat(key, newWord);
            len = (int) strlen(key);
            key[len] = CLUSTERSEP;
            key[len + 1] = 0;
            last = i + 1;
          }
        }
      }
      
      if (!last)
      {
        /* !last means there is no frequent word in this line. */
        continue;
      }
      
      hash = str2hash(key, pParam->clusterSketchSize,
              pParam->clusterSketchSeed);
      pParam->pClusterSketch[hash]++;
    }
    
    fclose(pFile);
  }
  
  oversupport = 0;
  for (j = 0; j < pParam->clusterSketchSize; j++)
  {
    if (pParam->pClusterSketch[j] >= pParam->support)
    {
      oversupport++;
    }
  }
  
  return oversupport;
}

/* When making changes to this function, don't forget to also change all four
 brother functions: create_cluster_candidates(), 
 create_cluster_candidates_with_wfilter(),
 create_cluster_candidates_word_dep(), and 
 create_cluster_candidates_word_dep_with_filter().
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_cluster_candidates_word_dep(struct Parameters 
  *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int wildcard[MAXWORDS + 1];
  int len, wordcount, i, constants, variables;
  struct Elem *pWord, *pElem;
  struct Elem *pStorage[MAXWORDS + 1];
  wordnumber_t clusterCount;
  
  //wordDep
  //wordnumber_t wordNumberStorage[MAXWORDS + 1];
  wordnumber_t p, q;
  int distinctConstants;
  
  
  for (j = 0; j < pParam->clusterTableSize; j++)
  {
    pParam->ppClusterTable[j] = 0;
  }
  
  for (p = 0; p < pParam->wordDepMatrixBreadth; p++)
  {
    for (q = 0; q < pParam->wordDepMatrixBreadth; q++)
    {
      pParam->wordDepMatrix[p * pParam->wordDepMatrixBreadth + q] = 0;
    }
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
      
      *key = 0;
      constants = 0;
      variables = 0;
      
      //wordDep
      distinctConstants = 0;
      
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          
          constants++;
          pStorage[constants] = pWord;
          wildcard[constants] = variables;
          variables = 0;
          
          //wordDep
          distinctConstants++;
          //findRepeated..
          if (is_word_repeated(pParam->wordNumStr, pWord->number,
                     distinctConstants))
          {
            distinctConstants--;
          }
          else
          {
            pParam->wordNumStr[distinctConstants] = pWord->number;
          }
        }
        else
        {
          variables++;
        }
      }
      
      //Deal with tail.
      //wildcard[constants - 1 + 1] = variables;
      wildcard[0] = variables;
      
      if (!constants)
      {
        continue;
      }
      
      //wordDep
      //update wordDep matrix
      update_word_dep_matrix(pParam->wordNumStr, distinctConstants,
                   pParam);
      
      if (pParam->clusterSketchSize)
      {
        hash = str2hash(key, pParam->clusterSketchSize,
                pParam->clusterSketchSeed);
        if (pParam->pClusterSketch[hash] < pParam->support)
        {
          continue;
        }
      }
      
      //Put this cluster into clustertable.
      pElem = add_elem(key, pParam->ppClusterTable,
               pParam->clusterTableSize, pParam->clusterTableSeed,
               pParam);
      
      if (pElem->count == 1)
      {
        clusterCount++;
        create_cluster_instance(pElem, constants, wildcard, pStorage,
                    pParam);
      }
      
      adjust_cluster_instance(pElem, constants, wildcard, pParam);
      
    }
    
    fclose(pFile);
  }
  
  return clusterCount;
}

/* This is a redundant function, which works similarly as function
 create_cluster_candidates_word_dep(), but with consideration of '--wfilter'
 option. Since this function has a coding style with overlapping IF-s, it could
 be read while comparing function create_cluster_candidates_word_dep() to see
 the differences. */

/* This redundant function can be integrated into its original function.
 However, for the sake of performance and readability of the original function,
 this function is separated as an alone function. */

/* In program, if '--wfilter' option is not used, this function will not be
 called. */

/* When making changes to this function, don't forget to also change all four
 brother functions: create_cluster_candidates(), 
 create_cluster_candidates_with_wfilter(),
 create_cluster_candidates_word_dep(), and 
 create_cluster_candidates_word_dep_with_filter().
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_cluster_candidates_word_dep_with_filter(
  struct Parameters *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int wildcard[MAXWORDS + 1];
  int len, wordcount, i, constants, variables;
  struct Elem *pWord, *pElem;
  struct Elem *pStorage[MAXWORDS + 1];
  wordnumber_t clusterCount;
  char newWord[MAXWORDLEN];
  
  //wordDep
  //wordnumber_t wordNumberStorage[MAXWORDS + 1];
  wordnumber_t p, q;
  int distinctConstants;
  
  *newWord = 0;
  for (j = 0; j < pParam->clusterTableSize; j++)
  {
    pParam->ppClusterTable[j] = 0;
  }
  
  for (p = 0; p < pParam->wordDepMatrixBreadth; p++)
  {
    for (q = 0; q < pParam->wordDepMatrixBreadth; q++)
    {
      pParam->wordDepMatrix[p * pParam->wordDepMatrixBreadth + q] = 0;
    }
  }
  
  
  for (pFilePtr = pParam->pInputFiles; pFilePtr; pFilePtr = pFilePtr->pNext)
  {
    if (!(pFile = fopen(pFilePtr->pName, "r")))
    {
      sprintf(logStr, "Can't open inputfile %s", pFilePtr->pName);
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
      
      *key = 0;
      constants = 0;
      variables = 0;
      
      //wordDep
      distinctConstants = 0;
      
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          
          constants++;
          pStorage[constants] = pWord;
          wildcard[constants] = variables;
          variables = 0;
          
          //wordDep
          distinctConstants++;
          //findRepeated..
          if (is_word_repeated(pParam->wordNumStr, pWord->number,
                     distinctConstants))
          {
            distinctConstants--;
          }
          else
          {
            pParam->wordNumStr[distinctConstants] = pWord->number;
          }
          
        }
        else if (is_word_filtered(words[i], pParam))
        {
          strcpy(newWord, word_search_replace(words[i], pParam));
          pWord = find_elem(newWord, pParam->ppWordTable,
                    pParam->wordTableSize,
                    pParam->wordTableSeed);
          if (words[i][0] != 0 && pWord)
          {
            strcat(key, newWord);
            len = (int) strlen(key);
            key[len] = CLUSTERSEP;
            key[len + 1] = 0;
            
            constants++;
            pStorage[constants] = pWord;
            wildcard[constants] = variables;
            variables = 0;
            
            //wordDep
            distinctConstants++;
            //findRepeated..
            if (is_word_repeated(pParam->wordNumStr,
                       pWord->number,
                       distinctConstants))
            {
              distinctConstants--;
            }
            else
            {
              pParam->wordNumStr[distinctConstants] =
              pWord->number;
            }
          }
          else
          {
            variables++;
          }
        }
        else
        {
          variables++;
        }
      }
      
      //Deal with tail.
      //wildcard[constants - 1 + 1] = variables;
      wildcard[0] = variables;
      
      if (!constants)
      {
        continue;
      }
      
      //wordDep
      //update wordDep matrix
      update_word_dep_matrix(pParam->wordNumStr, distinctConstants,
                   pParam);
      
      if (pParam->clusterSketchSize)
      {
        hash = str2hash(key, pParam->clusterSketchSize, 
                pParam->clusterSketchSeed);
        if (pParam->pClusterSketch[hash] < pParam->support)
        {
          continue;
        }
      }
      
      //Put this cluster into clustertable.
      pElem = add_elem(key, pParam->ppClusterTable,
               pParam->clusterTableSize, pParam->clusterTableSeed,
               pParam);
      
      if (pElem->count == 1)
      {
        clusterCount++;
        create_cluster_instance(pElem, constants, wildcard, pStorage,
                    pParam);
      }
      
      adjust_cluster_instance(pElem, constants, wildcard, pParam);
      
    }
    
    fclose(pFile);
  }
  
  return clusterCount;
}

/* When making changes to this function, don't forget to also change all four
 brother functions: create_cluster_candidates(), 
 create_cluster_candidates_with_wfilter(),
 create_cluster_candidates_word_dep(), and 
 create_cluster_candidates_word_dep_with_filter().
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_cluster_candidates(struct Parameters *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int wildcard[MAXWORDS + 1];
  int len, wordcount, i, constants, variables;
  struct Elem *pWord, *pElem;
  struct Elem *pStorage[MAXWORDS + 1];
  wordnumber_t clusterCount;
  
  for (j = 0; j < pParam->clusterTableSize; j++)
  {
    pParam->ppClusterTable[j] = 0;
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
      
      *key = 0;
      constants = 0;
      variables = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          
          constants++;
          pStorage[constants] = pWord;
          wildcard[constants] = variables;
          variables = 0;
        }
        else
        {
          variables++;
        }
      }
      
      //Deal with tail.
      //wildcard[constants - 1 + 1] = variables;
      wildcard[0] = variables;
      
      if (!constants)
      {
        continue;
      }
      
      if (pParam->clusterSketchSize)
      {
        hash = str2hash(key, pParam->clusterSketchSize,
                pParam->clusterSketchSeed);
        if (pParam->pClusterSketch[hash] < pParam->support)
        {
          continue;
        }
      }
      
      //Put this cluster into clustertable.
      pElem = add_elem(key, pParam->ppClusterTable,
               pParam->clusterTableSize, pParam->clusterTableSeed,
               pParam);
      
      if (pElem->count == 1)
      {
        clusterCount++;
        create_cluster_instance(pElem, constants, wildcard, pStorage,
                    pParam);
      }
      
      adjust_cluster_instance(pElem, constants, wildcard, pParam);
      
    }
    
    fclose(pFile);
  }
  
  return clusterCount;
}

/* This is a redundant function, which works similarly as function
 create_cluster_candidates(), but with consideration of '--wfilter'
 option. Since this function has a coding style with overlapping IF-s, it could
 be read while comparing function create_cluster_candidates() to see the
 differences. */

/* This redundant function can be integrated into its original function.
 However, for the sake of performance and readability of the original function,
 this function is separated as an alone function. */

/* In program, if '--wfilter' option is not used, this function will not be
 called. */

/* When making changes to this function, don't forget to also change all four
 brother functions: create_cluster_candidates(), 
 create_cluster_candidates_with_wfilter(),
 create_cluster_candidates_word_dep(), and 
 create_cluster_candidates_word_dep_with_filter().
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static wordnumber_t create_cluster_candidates_with_wfilter(struct Parameters 
  *pParam)
{
  FILE *pFile;
  struct InputFile *pFilePtr;
  tableindex_t j, hash;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char words[MAXWORDS][MAXWORDLEN];
  char key[MAXKEYLEN];
  int wildcard[MAXWORDS + 1];
  int len, wordcount, i, constants, variables;
  struct Elem *pWord, *pElem;
  struct Elem *pStorage[MAXWORDS + 1];
  wordnumber_t clusterCount;
  char newWord[MAXWORDLEN];
  
  *newWord = 0;
  
  for (j = 0; j < pParam->clusterTableSize; j++)
  {
    pParam->ppClusterTable[j] = 0;
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
      
      *key = 0;
      constants = 0;
      variables = 0;
      
      for (i = 0; i < wordcount; i++)
      {
        pWord = find_elem(words[i], pParam->ppWordTable,
                  pParam->wordTableSize, pParam->wordTableSeed);
        if (words[i][0] != 0 && pWord)
        {
          strcat(key, words[i]);
          len = (int) strlen(key);
          key[len] = CLUSTERSEP;
          key[len + 1] = 0;
          
          constants++;
          pStorage[constants] = pWord;
          wildcard[constants] = variables;
          variables = 0;
        }
        else if(is_word_filtered(words[i], pParam))
        {
          strcpy(newWord, word_search_replace(words[i], pParam));
          pWord = find_elem(newWord, pParam->ppWordTable,
                    pParam->wordTableSize,
                    pParam->wordTableSeed);
          if (words[i][0] != 0 && pWord)
          {
            strcat(key, newWord);
            len = (int) strlen(key);
            key[len] = CLUSTERSEP;
            key[len + 1] = 0;
            
            constants++;
            pStorage[constants] = pWord;
            wildcard[constants] = variables;
            variables = 0;
          }
          else
          {
            variables++;
          }
        }
        else
        {
          variables++;
        }
      }
      
      //Deal with tail.
      //wildcard[constants - 1 + 1] = variables;
      wildcard[0] = variables;
      
      if (!constants)
      {
        continue;
      }
      
      if (pParam->clusterSketchSize)
      {
        hash = str2hash(key, pParam->clusterSketchSize,
                pParam->clusterSketchSeed);
        if (pParam->pClusterSketch[hash] < pParam->support)
        {
          continue;
        }
      }
      
      //Put this cluster into clustertable.
      pElem = add_elem(key, pParam->ppClusterTable,
               pParam->clusterTableSize, pParam->clusterTableSeed,
               pParam);
      
      if (pElem->count == 1)
      {
        clusterCount++;
        create_cluster_instance(pElem, constants, wildcard, pStorage,
                    pParam);
      }
      
      adjust_cluster_instance(pElem, constants, wildcard, pParam);
      
    }
    
    fclose(pFile);
  }
  
  return clusterCount;
}

static struct Cluster *create_cluster_instance(struct Elem* pClusterElem,
                    int constants, int wildcard[],
                    struct Elem *pStorage[],
                    struct Parameters *pParam)
{
  struct Cluster *ptr;
  int i = 0;
  
  ptr = (struct Cluster *) malloc(sizeof(struct Cluster));
  
  if (!ptr)
  {
    log_msg(MALLOC_ERR_6009, LOG_ERR, pParam);
    exit(1);
  }
  
  ptr->ppWord = (struct Elem **) malloc((constants + 1) *
                      sizeof(struct Elem *));
  if (!ptr->ppWord)
  {
    log_msg(MALLOC_ERR_6009, LOG_ERR, pParam);
    exit(1);
  }
  
  ptr->fullWildcard = (int *) malloc(2 * (constants + 1) * sizeof(int));

  if (!ptr->fullWildcard)
  {
    log_msg(MALLOC_ERR_6009, LOG_ERR, pParam);
    exit(1);
  }
  
  //Initializtion..
  ptr->ppWord[0] = 0; //reserved..
  for (i = 1; i <= constants; i++)
  {
    ptr->ppWord[i] = pStorage[i];
    ptr->fullWildcard[i * 2] = wildcard[i];
    ptr->fullWildcard[i * 2 + 1] = wildcard[i];
  }
  
  ptr->fullWildcard[0] = wildcard[0];
  ptr->fullWildcard[1] = wildcard[0];
  
  ptr->constants = constants;
  ptr->count = 0;
  ptr->bIsJoined = 0;
  ptr->pLastNode = 0;
  
  //Build bidirectional link.
  pClusterElem->pCluster = ptr;
  ptr->pElem = pClusterElem;
  
  //Find a more organized place to store the new pointers of struct Cluster.
  if (pParam->pClusterFamily[constants])
  {
    ptr->pNext = pParam->pClusterFamily[constants];
    pParam->pClusterFamily[constants] = ptr;
  }
  else
  {
    ptr->pNext = 0;
    pParam->pClusterFamily[constants] = ptr;
  }
  
  if (constants > pParam->biggestConstants)
  {
    /* biggestConstants saves time for later iteration. */
    pParam->biggestConstants = constants;
  }
  
  return ptr;
}

/* Adjust the minimum and maximum of the wildcards. */
static void adjust_cluster_instance(struct Elem* pClusterElem, int constants,
               int wildcard[], struct Parameters *pParam)
{
  struct Cluster *ptr;
  int i;
  
  ptr = pClusterElem->pCluster;
  ptr->count++;
  
  for (i = 0; i <= constants; i++)
  {
    if (wildcard[i] < ptr->fullWildcard[i * 2])
    {
      ptr->fullWildcard[i * 2] = wildcard[i];
    }
    else if (wildcard[i] > ptr->fullWildcard[i * 2 + 1])
    {
      ptr->fullWildcard[i * 2 + 1] = wildcard[i];
    }
    
  }
  
}