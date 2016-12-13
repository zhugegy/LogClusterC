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
 * File:   join_clusters_heuristic.c
 * 
 * Content: Functions related to join_clusters heuristic.
 *
 * Created on November 30, 2016, 6:43 AM
 */

#include "common_header.h"
#include "join_clusters_heuristic.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "output.h"
#include "hash_table_processing.h"
#include "utility.h"
#include "line_processing.h"

static void set_token(struct Parameters *pParam);
static void join_cluster(struct Parameters *pParam);
static void check_cluster_for_join_cluster(struct Cluster* pCluster,
                  struct Parameters *pParam);
static double cal_word_weight(struct Cluster *pCluster, int serial,
             struct Parameters *pParam);
static double cal_word_weight_function_1(struct Cluster *pCluster, int serial,
                  struct Parameters *pParam);
static double cal_word_weight_function_2(struct Cluster *pCluster, int serial,
                  struct Parameters *pParam);
static double cal_word_dep(struct Elem *word1, struct Elem *word2,
          struct Parameters *pParam);
static double cal_word_dep_number_version(wordnumber_t word1num, 
        wordnumber_t word2num, struct Parameters *pParam);
static void get_unique_frequent_words_out_of_cluster(struct Cluster *pCluster,
                        struct Parameters *pParam);
static void join_cluster_with_token(struct Cluster *pCluster,
               struct Parameters *pParam);
static struct ClusterWithToken *create_cluster_with_token_instance(
  struct Cluster *pCluster, struct Elem *pElem, struct Parameters *pParam);
static void adjust_cluster_with_token_instance(struct Cluster *pCluster,
                    struct Elem *pElem, struct Parameters *pParam);
static int check_if_token_key_is_exist(struct ClusterWithToken *ptr, int serial,
                struct Elem *pElem);

void update_word_dep_matrix(wordnumber_t *storage, int serial,
              struct Parameters *pParam)
{
  int i, j;
  //unsigned long long coor;
  
  for (i = 1; i <= serial; i++)
  {
    for (j = 1; j <= serial; j++)
    {
      //coor = storage[i] * pParam->wordDepMatrixBreadth + storage[j];
      pParam->wordDepMatrix[storage[i] * pParam->wordDepMatrixBreadth +
                  storage[j]]++;
    }
  }
}

void step_3_join_clusters(struct Parameters *pParam)
{
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  log_msg("Joining clusters...", LOG_NOTICE, pParam);
  
  set_token(pParam);
  
  join_cluster(pParam);
  
  str_format_int_grouped(digit, pParam->joinedClusterInputNum);
  sprintf(logStr, "%s clusters contain frequent words under word weight"
      "threshold.", digit);
  log_msg(logStr, LOG_INFO, pParam);
  
  str_format_int_grouped(digit, pParam->joinedClusterOutputNum);
  sprintf(logStr, "Those clusters were joined into %s clusters.", digit);
  log_msg(logStr, LOG_INFO, pParam);
}

/* If the default token, which is "token", is already among frequent words,
 generate random string to replace "token". */
static void set_token(struct Parameters *pParam)
{
  while (find_elem(pParam->token, pParam->ppWordTable, pParam->wordTableSize,
           pParam->wordTableSeed))
  {
    gen_random_string(pParam->token, TOKENLEN - 1);
  }
}

static void join_cluster(struct Parameters *pParam)
{
  int i;
  struct Cluster *pCluster;
  struct ClusterWithToken *pClusterWithToken;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    pCluster = pParam->pClusterFamily[i];
    
    while (pCluster)
    {
      check_cluster_for_join_cluster(pCluster, pParam);
      pCluster = pCluster->pNext;
    }
  }
  
  //additional work. Equal the counters in Elem and ClusterWithToken
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    pClusterWithToken = pParam->pClusterWithTokenFamily[i];
    
    while (pClusterWithToken)
    {
      pClusterWithToken->pElem->count = pClusterWithToken->count;
      pClusterWithToken = pClusterWithToken->pNext;
    }
  }
}

static void check_cluster_for_join_cluster(struct Cluster* pCluster,
                  struct Parameters *pParam)
{
  int i;
  
  for (i = 0; i <= pCluster->constants; i++)
  {
    pParam->tokenMarker[i] = 0;
  }
  
  for (i = 1; i <= pCluster->constants ; i++)
  {
    if (cal_word_weight(pCluster, i, pParam) < pParam->wordWeightThreshold)
    {
      /* tokenMarker[0] means this cluster has token. We should keep on
       to see which constant(s) is/are token(s). */
      pParam->tokenMarker[0] = 1;
      
      pParam->tokenMarker[i] = 1;
    }
  }
  
  if (pParam->tokenMarker[0] == 1)
  {
    pCluster->bIsJoined = 1;
    join_cluster_with_token(pCluster, pParam);
  }
}

static double cal_word_weight(struct Cluster *pCluster, int serial,
             struct Parameters *pParam)
{
  switch (pParam->wordWeightFunction)
  {
    case 1:
      return cal_word_weight_function_1(pCluster, serial, pParam);
      break;
    case 2:
      return cal_word_weight_function_2(pCluster, serial, pParam);
      break;
    default:
      log_msg("failed calculate word weight. Funciton: cal_word_weight()",
          LOG_ERR, pParam);
      exit(1);
      break;
  }
  
  return 0;
}

static double cal_word_weight_function_1(struct Cluster *pCluster, int serial,
                  struct Parameters *pParam)
{
  double sum;
  int i;
  double result;
  
  sum = 0;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    sum += cal_word_dep(pCluster->ppWord[i], pCluster->ppWord[serial],
              pParam);
  }
  
  result = sum / pCluster->constants;
  
  return result;
}

static double cal_word_weight_function_2(struct Cluster *pCluster, int serial,
                  struct Parameters *pParam)
{
  double sum;
  int i;
  double result;
  
  sum = 0;
  
  if (pCluster != pParam->pCurrentCluster)
  {
    //get all unique frequent words
    get_unique_frequent_words_out_of_cluster(pCluster, pParam);
  }
  
  if (pParam->wordNumStr[0] < 1)
  {
    i = 0; //debugpoint;
  }
  
  if (pParam->wordNumStr[0] == 1)
  {
    i = 0; //debug breakpoint
    return 1;
  }
  
  for (i = 1; i <= pParam->wordNumStr[0]; i++)
  {
    sum += cal_word_dep_number_version(pParam->wordNumStr[i],
                       pCluster->ppWord[serial]->number, pParam);
  }
  
  result = (double) (sum - 1) / (pParam->wordNumStr[0] - 1);
  
  return result;
}

static double cal_word_dep(struct Elem *word1, struct Elem *word2,
          struct Parameters *pParam)
{
  double dependency;
  
  //how many times word1 appears in log files.
  wordnumber_t word1Total;
  
  //how many times word2 appears with word1.
  wordnumber_t word2NumInWord1;
  
  
  word1Total = pParam->wordDepMatrix[word1->number *
                     pParam->wordDepMatrixBreadth +
                     word1->number];
  
  word2NumInWord1 = pParam->wordDepMatrix[word1->number *
                      pParam->wordDepMatrixBreadth +
                      word2->number];
  
  dependency = (double) word2NumInWord1 / word1Total;
  
  return dependency;
}

static void get_unique_frequent_words_out_of_cluster(struct Cluster *pCluster,
                        struct Parameters *pParam)
{
  int i;
  int distinctConstants;
  
  distinctConstants = 0;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    distinctConstants++;
    if (is_word_repeated(pParam->wordNumStr, pCluster->ppWord[i]->number,
               distinctConstants))
    {
      distinctConstants--;
    }
    else
    {
      pParam->wordNumStr[i] = pCluster->ppWord[i]->number;
    }
  }
  pParam->wordNumStr[0] = distinctConstants;
  
  pParam->pCurrentCluster = pCluster;
}

/* Redundant function. Parameters are words, instead of the {struct Elem}
 pointer. */
static double cal_word_dep_number_version(wordnumber_t word1num, 
        wordnumber_t word2num, struct Parameters *pParam)
{
  double dependency;
  
  //how many times word1 appears in log files.
  wordnumber_t word1Total;
  
  //how many times word2 appears with word1.
  wordnumber_t word2NumInWord1;
  
  
  word1Total = pParam->wordDepMatrix[word1num * pParam->wordDepMatrixBreadth +
                     word1num];
  
  word2NumInWord1 = pParam->wordDepMatrix[word1num *
                      pParam->wordDepMatrixBreadth +
                      word2num];
  
  dependency = (double) word2NumInWord1 / word1Total;
  
  return dependency;
}

static void join_cluster_with_token(struct Cluster *pCluster,
               struct Parameters *pParam)
{
  char key[MAXKEYLEN];
  int i, len;
  struct Elem *pElem;
  
  pParam->joinedClusterInputNum++;
  
  *key = 0;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    if (pParam->tokenMarker[i] == 0)
    {
      strcat(key, pCluster->ppWord[i]->pKey);
    }
    else
    {
      strcat(key, pParam->token);
    }
    len = (int) strlen(key);
    key[len] = CLUSTERSEP;
    key[len + 1] = 0;
  }
  
  pElem = add_elem(key, pParam->ppClusterTable, pParam->clusterTableSize,
           pParam->clusterTableSeed, pParam);
  
  if (pElem->count == 1)
  {
    pParam->joinedClusterOutputNum++;
    //create cluster_with_token instance
    create_cluster_with_token_instance(pCluster, pElem, pParam);
  }
  
  
  //adjust this instance
  adjust_cluster_with_token_instance(pCluster, pElem, pParam);
}

static struct ClusterWithToken *create_cluster_with_token_instance(
  struct Cluster *pCluster, struct Elem *pElem, struct Parameters *pParam)
{
  struct ClusterWithToken *ptr;
  int i;
  
  ptr = (struct ClusterWithToken *) malloc(sizeof(struct ClusterWithToken));
  if (!ptr)
  {
    log_msg(MALLOC_ERR_6010, LOG_ERR, pParam);
    exit(1);
  }
  
  ptr->ppWord = (struct Elem **) malloc((pCluster->constants + 1) * 
          sizeof(struct Elem *));
  if (!ptr->ppWord)
  {
    log_msg(MALLOC_ERR_6010, LOG_ERR, pParam);
    exit(1);
  }
  
  ptr->ppToken = (struct Token **) malloc((pCluster->constants + 1) * 
          sizeof(struct Token *));
  if (!ptr->ppToken)
  {
    log_msg(MALLOC_ERR_6010, LOG_ERR, pParam);
    exit(1);
  }
  
	ptr->fullWildcard = (int *) malloc(2 * (pCluster->constants + 1) * 
          sizeof(int));
  if (!ptr->fullWildcard)
  {
    log_msg(MALLOC_ERR_6010, LOG_ERR, pParam);
    exit(1);
  }
	
  //Initialization..
  ptr->ppWord[0] = 0; //reserved..
  ptr->ppToken[0] = 0; //reserved..
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    ptr->ppWord[i] = pCluster->ppWord[i];
    ptr->fullWildcard[i * 2] = pCluster->fullWildcard[i * 2];
    ptr->fullWildcard[i * 2 + 1] = pCluster->fullWildcard[i * 2 + 1];
    ptr->ppToken[i] = 0;
  }
  
  ptr->fullWildcard[0] = pCluster->fullWildcard[0];
  ptr->fullWildcard[1] = pCluster->fullWildcard[1];
  
  ptr->constants = pCluster->constants;
  ptr->count = 0;
  ptr->bIsJoined = pCluster->bIsJoined;
  ptr->pLastNode = pCluster->pLastNode;
  
  //Build bidirectional link.
  //Type converted to (struct Cluster *) here. Should not cause a probelm.
  pElem->pCluster = (struct Cluster *) ptr;
  ptr->pElem = pElem;
  
  //Find a organized palce to store the ptrs.
  if (pParam->pClusterWithTokenFamily[ptr->constants])
  {
    ptr->pNext = pParam->pClusterWithTokenFamily[ptr->constants];
    pParam->pClusterWithTokenFamily[ptr->constants] = ptr;
  }
  else
  {
    ptr->pNext = 0;
    pParam->pClusterWithTokenFamily[ptr->constants] = ptr;
  }
  
  return ptr;
}

static void adjust_cluster_with_token_instance(struct Cluster *pCluster,
                    struct Elem *pElem, struct Parameters *pParam)
{
  struct ClusterWithToken *ptr;
  struct Token *ptrToken;
  int i;
  
  ptr = (struct ClusterWithToken *) pElem->pCluster;
  
  ptr->count += pCluster->count;
  
  for (i = 0; i <= ptr->constants; i++)
  {
    if (pCluster->fullWildcard[i * 2] < ptr->fullWildcard[i * 2])
    {
      ptr->fullWildcard[i * 2] = pCluster->fullWildcard[i * 2];
    }
    
    if (pCluster->fullWildcard[i * 2 + 1] > ptr->fullWildcard[i * 2 + 1])
    {
      ptr->fullWildcard[i * 2 + 1] = pCluster->fullWildcard[i * 2 + 1];
    }
  }
  
  for (i = 1; i <= ptr->constants; i++)
  {
    if (pParam->tokenMarker[i] == 1)
    {
      //debug here..20160224
      
      if (check_if_token_key_is_exist(ptr, i, pCluster->ppWord[i]))
      {
        //Repeated word will not be added as a new token.
        continue;
      }
      
      ptrToken = (struct Token *) malloc(sizeof(struct Token));
      if (!ptrToken)
      {
        log_msg(MALLOC_ERR_6011, LOG_ERR, pParam);
        exit(1);
      }
      ptrToken->pWord = pCluster->ppWord[i];
      
      if (ptr->ppToken[i])
      {
        ptrToken->pNext = ptr->ppToken[i];
        ptr->ppToken[i] = ptrToken;
      }
      else
      {
        ptrToken->pNext = 0;
        ptr->ppToken[i] = ptrToken;
      }
    }
  }
  
}

static int check_if_token_key_is_exist(struct ClusterWithToken *ptr, int serial,
                struct Elem *pElem)
{
  struct Token *pToken;
  
  pToken = ptr->ppToken[serial];
  while (pToken)
  {
    if (pToken->pWord == pElem)
    {
      return 1;
    }
    pToken = pToken->pNext;
  }
  
  return 0;
}