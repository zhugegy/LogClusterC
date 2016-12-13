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
 * File:   free_resource.c
 * 
 * Content: Functions related to free allocated heap memory.
 *
 * Created on November 29, 2016, 11:08 PM
 */

#include "common_header.h"
#include "free_resource.h"

#include <regex.h>     /* for regcomp() and regexec() */
#include <syslog.h>    /* for syslog() */

static void free_inputfiles(struct Parameters *pParam);
static void free_delim(struct Parameters *pParam);
static void free_filter(struct Parameters *pParam);
static void free_template(struct Parameters *pParam);
static void free_outlier(struct Parameters *pParam);
static void free_wfilter(struct Parameters *pParam);
static void free_wsearch(struct Parameters *pParam);
static void free_wreplace(struct Parameters *pParam);
static void free_word_table(struct Parameters *pParam);
static void free_word_sketch(struct Parameters *pParam);
static void free_cluster_table(struct Parameters *pParam);
static void free_cluster_sketch(struct Parameters *pParam);
static void free_cluster_instances(struct Parameters *pParam);
static void free_cluster_with_token_instances(struct Parameters *pParam);
static void free_token(struct ClusterWithToken *pClusterWithToken);


void free_syslog_facility(struct Parameters *pParam)
{
  free((void *) pParam->pSyslogFacility);
}

//This function can cause segment 11 error when trie is large. Thus it is not 
//used. For more details, see the comments of function 
//step_2_aggregate_supports();
//void free_trie_nodes(struct TrieNode *pNode, struct Parameters *pParam)
//{
//  struct TrieNode *pNext, *pParent;
//  
//  if (pNode == 0)
//  {
//    return;
//  }
//  
//  /*
//   if (pParam->pPrefixRoot == 0)
//   {
//   return;
//   }
//   */
//  
//  //ptr = pNode;
//  
//  //debug purpose
//  //char *tmp = "null11";
//  //debug purpose...
//  
//  while (pNode)
//  {
//    free_trie_nodes(pNode->pChild, pParam);
//    
//    if (pParam->pPrefixRoot == 0)
//    {
//      return;
//    }
//    
//    if (pNode != pParam->pPrefixRoot)
//    {
//      pNode->pParent->pChild = pNode->pNext;
//    }
//    
//    pNext = pNode->pNext;
//    pParent = pNode->pParent;
//    
//    //debug purpose
//    //if (pNode->pWord) {
//    //  log_msg(pNode->pWord->pKey, LOG_INFO, pParam);
//    
//    //}
//    //else
//    //{
//    //  log_msg(tmp, LOG_INFO, pParam);
//    //}
//    //debug purpose...
//    
//    if (pNode == pParam->pPrefixRoot)
//    {
//      pParam->pPrefixRoot = 0;
//    }
//    
//    free((void *) pNode);
//    pNode = pNext;
//    if (!pNode)
//    {
//      pNode =pParent;
//      //pParent->pChild = 0;
//    }
//  }
//  
//}

void free_and_clean_step_0(struct Parameters *pParam)
{
  free_inputfiles(pParam);
  free_syslog_facility(pParam);
  free_delim(pParam);
  free_filter(pParam);
  free_template(pParam);
  free_outlier(pParam);
  free_wfilter(pParam);
  free_wsearch(pParam);
  free_wreplace(pParam);
  if (pParam->bSyslogFlag == 1)
  {
    closelog();
  }
}

void free_and_clean_step_1(struct Parameters *pParam)
{
  free_word_table(pParam);
  free_word_sketch(pParam);
}

void free_and_clean_step_2(struct Parameters *pParam)
{
  free_cluster_table(pParam);
  free_cluster_sketch(pParam);
  free_cluster_instances(pParam);
  if (pParam->wordWeightThreshold)
  {
    free((void *) pParam->wordDepMatrix);
  }
}

void free_and_clean_step_3(struct Parameters *pParam)
{
  if (pParam->wordWeightThreshold)
  {
    free_cluster_with_token_instances(pParam);
  }
}

static void free_inputfiles(struct Parameters *pParam)
{
  struct InputFile *ptr, *pNext;
  
  ptr = pParam->pInputFiles;
  
  while (ptr)
  {
    pNext = ptr->pNext;
    free((void *) ptr->pName);
    free((void *) ptr);
    ptr = pNext;
  }
}

static void free_delim(struct Parameters *pParam)
{
  regfree(&pParam->delim_regex);
  if (pParam->pDelim)
  {
    free((void *) pParam->pDelim);
  }
}

static void free_filter(struct Parameters *pParam)
{
  if (pParam->pFilter)
  {
    regfree(&pParam->filter_regex);
    free((void *) pParam->pFilter);
  }
  
}

static void free_template(struct Parameters *pParam)
{
  struct TemplElem *ptr, *pNext;
  
  ptr = pParam->pTemplate;
  
  while (ptr)
  {
    pNext = ptr->pNext;
    free((void *) ptr->pStr);
    free((void *) ptr);
    ptr = pNext;
  }
}

static void free_outlier(struct Parameters *pParam)
{
  if (pParam->pOutlier)
  {
    free((void *) pParam->pOutlier);
  }
}

static void free_wfilter(struct Parameters *pParam)
{
  if (pParam->pWordFilter)
  {
    regfree(&pParam->wfilter_regex);
    free((void *) pParam->pWordFilter);
  }
}

static void free_wsearch(struct Parameters *pParam)
{
  if (pParam->pWordSearch)
  {
    regfree(&pParam->wsearch_regex);
    free((void *) pParam->pWordSearch);
  }
}

static void free_wreplace(struct Parameters *pParam)
{
  if (pParam->pWordReplace)
  {
    free((void *) pParam->pWordReplace);
  }
}

static void free_word_table(struct Parameters *pParam)
{
  tableindex_t i;
  struct Elem *ptr, *pNext;
  
  for (i = 0; i < pParam->wordTableSize; ++i)
  {
    
    if (!pParam->ppWordTable[i])  { continue; }
    
    ptr = pParam->ppWordTable[i];
    
    while (ptr)
    {
      
      pNext = ptr->pNext;
      
      free((void *) ptr->pKey);
      free((void *) ptr);
      
      ptr = pNext;
      
    }
    
  }
  
  free((void *) pParam->ppWordTable);
}

static void free_word_sketch(struct Parameters *pParam)
{
  if (pParam->pWordSketch)
  {
    free((void *) pParam->pWordSketch);
  }
  
}

static void free_cluster_table(struct Parameters *pParam)
{
  tableindex_t i;
  struct Elem *ptr, *pNext;
  
  if (pParam->ppClusterTable)
  {
    for (i = 0; i < pParam->clusterTableSize; ++i)
    {
      
      if (!pParam->ppClusterTable[i])  { continue; }
      
      ptr = pParam->ppClusterTable[i];
      
      while (ptr)
      {
        pNext = ptr->pNext;
        
        free((void *) ptr->pKey);
        free((void *) ptr);
        
        ptr = pNext;
      }
    }
    
    free((void *) pParam->ppClusterTable);
  }
  
}

static void free_cluster_sketch(struct Parameters *pParam)
{
  if (pParam->pClusterSketch)
  {
    free((void *) pParam->pClusterSketch);
  }
}

static void free_cluster_instances(struct Parameters *pParam)
{
  int i;
  struct Cluster *ptr, *pNext;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterFamily[i];
    while (ptr)
    {
      pNext = ptr->pNext;
      free((void *) ptr->ppWord);
      free((void *) ptr->fullWildcard);
      free((void *) ptr);
      ptr = pNext;
    }
  }
}

static void free_cluster_with_token_instances(struct Parameters *pParam)
{
  int i;
  struct ClusterWithToken *ptr, *pNext;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterWithTokenFamily[i];
    
    while (ptr)
    {
      pNext = ptr->pNext;
      free_token(ptr);
      free((void *) ptr->ppToken);
      free((void *) ptr->ppWord);
      free((void *) ptr->fullWildcard);
      free((void *) ptr);
      ptr = pNext;
    }
  }
}

static void free_token(struct ClusterWithToken *pClusterWithToken)
{
  int i;
  struct Token *ptr, *pNext;
  
  for (i = 1; i <= pClusterWithToken->constants; i++)
  {
    ptr = pClusterWithToken->ppToken[i];
    
    while (ptr)
    {
      pNext = ptr->pNext;
      free((void *) ptr);
      ptr = pNext;
    }
  }
}