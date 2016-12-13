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
 * File:   aggregate_supports_heuristic.c
 * 
 * Content: Functions related to the aggregate_supports heuristic.
 *
 * Created on November 30, 2016, 7:31 AM
 */

#include "common_header.h"
#include "aggregate_supports_heuristic.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "output.h"
#include "utility.h"

static struct TrieNode *build_prefix_trie(struct Parameters *pParam);
static void insert_cluster_into_trie(struct TrieNode *pRoot, struct Cluster 
  *pCluster, struct Parameters *pParam);
static int insert_cluster_into_trie_wildcard(struct TrieNode *pParent, int min,
                    int max, struct Parameters *pParam);
static int insert_cluster_into_trie_word(struct TrieNode *pParent, 
        struct Elem *pWord, struct Parameters *pParam);
static struct TrieNode *create_trie_node(struct Elem *pElem, 
        struct TrieNode *pParent, struct TrieNode *pPrev, 
        struct Parameters *pParam);
static void aggregate_candidates(struct Parameters *pParam);
static int get_first_wildcard_location(struct Cluster *pCluster);
static void aggregate_candidate(struct Cluster *pCluster, 
        struct Parameters *pParam);
static struct TrieNode *get_common_parent(struct Cluster *pCluster);
static int get_first_wildcard_reverse_depth(struct Cluster *pCluster);
static int find_more_specific(struct TrieNode *pParent, 
        struct Cluster *pCluster, int constant, int min, int max, 
        wordnumber_t hash, struct Parameters *pParam);
static int find_more_specific_tail(struct TrieNode *pParent, 
        struct Cluster*pCluster, int min, int max);

void step_2_aggregate_supports(struct Parameters *pParam)
{
  struct TrieNode *pRoot;
  
  log_msg("Aggregate cluster candidates...", LOG_NOTICE, pParam);
  pParam->prefixSketchSize = pParam->freWordNum * 3;
  pParam->wildcardHash = pParam->freWordNum * 3;
  
  pRoot = build_prefix_trie(pParam);
  
  aggregate_candidates(pParam);
  
  //debug purpose...
  //log_msg("Re-finding clusters...", LOG_INFO, &param);
  //clusterNum = find_clusters_from_candidates(&param);
  //str_format_int_grouped(digit, clusterNum);
  //sprintf(logStr, "%s cluster were found.", digit);
  //log_msg(logStr, LOG_INFO, &param);
  
  //2016-04-12 commented out free_trie_nodes(pRoot, pParam);
  //This function faces "segmentation 11" error when handling 
  //large trie nodes. 
  //(Or this is because the characteristic of specific event log
  //files that generate some weird cluster candidates, therefore
  //generating some weird trie nodes that are hard to free.)
  //Commenting out this function is not a problem.
  //System will take this issue and helps in freeing heap memory, 
  //when the process is terminated.
  //(Memory leaking is a affordable problem, as this program is not a long-time
  //background program that keeps consuming memory. Nevertheless, other heap
  //memory that won't cause errors when being released shall always be 
  //released.)
  
  //free_trie_nodes(pRoot, pParam);
}

/* This function iterates all cluster candiates and build the prefix tree. */
static struct TrieNode *build_prefix_trie(struct Parameters *pParam)
{
  int i = 0;
  struct Cluster *ptr;
  
  struct TrieNode *pRoot = (struct TrieNode *) malloc(sizeof(struct TrieNode));
  if (!pRoot)
  {
    log_msg(MALLOC_ERR_6003, LOG_ERR, pParam);
    exit(1);
  }
  
  pParam->trieNodeNum = 1;
  /* Root has unique id. */
  pRoot->hashValue = pParam->wildcardHash + 1;
  
  pRoot->pParent = 0;
  pRoot->pChild = 0;
  pRoot->pNext = 0;
  pRoot->pWord = 0;
  pRoot->wildcardMin = 0;
  pRoot->wildcardMax = 0;
  pRoot->pIsEnd = 0;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterFamily[i];
    while (ptr)
    {
      //build cluster into trie..
      insert_cluster_into_trie(pRoot, ptr, pParam);
      ptr = ptr->pNext;
    }
  }
  
  pParam->pPrefixRoot = pRoot;
  
  //Returning a local variable shall cause warning. Anyway, it has been stored 
  //in pParam->pPrefixRoot. 
  //PS: pRoot is not a local variable. It is in heap. 
  return pRoot;
}

static void insert_cluster_into_trie(struct TrieNode *pRoot, struct Cluster 
  *pCluster, struct Parameters *pParam)
{
  int i;
  int wildcardMin, wildcardMax;
  struct TrieNode *ptr;
  ptr = pRoot;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    if (pCluster->fullWildcard[i * 2 + 1] != 0)
    {
      //insert_wild_card
      wildcardMin = pCluster->fullWildcard[i * 2];
      wildcardMax = pCluster->fullWildcard[i * 2 + 1];
      if (insert_cluster_into_trie_wildcard(ptr, wildcardMin, wildcardMax,
                          pParam))
      {
        //Found
        ptr = pParam->pPrefixRet;
      }
      else
      {
        //Not found
        pParam->prefixWildcardMin = wildcardMin;
        pParam->prefixWildcardMax = wildcardMax;
        
        ptr = create_trie_node(0, ptr, pParam->pPrefixRet, pParam);
      }
      
      //insert_word
      if (insert_cluster_into_trie_word(ptr, pCluster->ppWord[i], pParam))
      {
        //Found
        ptr = pParam->pPrefixRet;
      }
      else
      {   //Not found
        ptr = create_trie_node(pCluster->ppWord[i], ptr,
                     pParam->pPrefixRet, pParam);
      }
      
    }
    else
    {
      //insert_word
      if (insert_cluster_into_trie_word(ptr, pCluster->ppWord[i], pParam))
      {
        //Found
        ptr = pParam->pPrefixRet;
      }
      else
      {   //Not found
        ptr = create_trie_node(pCluster->ppWord[i], ptr,
                     pParam->pPrefixRet, pParam);
      }
    }
  }
  
  // Deal with the tail.
  if (pCluster->fullWildcard[1] != 0)
  {
    //insert_wild_card
    wildcardMin = pCluster->fullWildcard[0];
    wildcardMax = pCluster->fullWildcard[1];
    if (insert_cluster_into_trie_wildcard(ptr, wildcardMin, wildcardMax,
                        pParam))
    {
      //Found
      ptr = pParam->pPrefixRet;
    }
    else
    {
      //Not found
      pParam->prefixWildcardMin = wildcardMin;
      pParam->prefixWildcardMax = wildcardMax;
      
      ptr = create_trie_node(0, ptr, pParam->pPrefixRet, pParam);
    }
  }
  
  ptr->pIsEnd = pCluster;
  pCluster->pLastNode = ptr;
}

/* Insert wildcard into trie. */
static int insert_cluster_into_trie_wildcard(struct TrieNode *pParent, int min,
                    int max, struct Parameters *pParam)
{
  struct TrieNode *ptr;
  ptr = pParent->pChild;
  
  while (ptr)
  {
    if (ptr->hashValue == pParam->wildcardHash)
    {
      if (ptr->wildcardMin == min && ptr->wildcardMax == max)
      {
        pParam->pPrefixRet = ptr;
        return 1;
      }
      else
      {
        ptr = ptr->pNext;
      }
      
    }
    else
    {
      pParam->pPrefixRet = 0;
      return 0;
    }
  }
  
  pParam->pPrefixRet = 0;
  return 0;
}

/* Insert constant into trie. */
static int insert_cluster_into_trie_word(struct TrieNode *pParent, 
        struct Elem *pWord, struct Parameters *pParam)
{
  wordnumber_t hash;
  struct TrieNode *ptr, *pPrev;
  
  hash = str2hash(pWord->pKey, pParam->prefixSketchSize,
          pParam->prefixSketchSeed);
  
  ptr = pParent->pChild;
  pPrev = 0;
  
  while (ptr)
  {
    if (ptr->hashValue > hash)
    {
      pPrev = ptr;
      ptr = ptr->pNext;
      continue;
    }
    
    if (ptr->hashValue == hash)
    {
      if (!strcmp(ptr->pWord->pKey, pWord->pKey))
      {
        pParam->pPrefixRet = ptr;
        return 1;
      }
      else
      {
        pPrev = ptr;
        ptr = ptr->pNext;
        continue;
      }
      
    }
    
    if (ptr->hashValue < hash)
    {
      pParam->pPrefixRet = pPrev;
      return 0;
    }
  }
  
  pParam->pPrefixRet = pPrev;
  return 0;
}

/* The first parameter indicates whether the node is constant or wildcard. If
 node is constant, it will be the pointer of {struct Elem}. If node is
 wildcard, it will be null(0). */
static struct TrieNode *create_trie_node(struct Elem *pElem, 
        struct TrieNode *pParent, struct TrieNode *pPrev, 
        struct Parameters *pParam)
{
  struct TrieNode *pNode = (struct TrieNode *) malloc(sizeof(struct TrieNode));
  if (!pNode)
  {
    log_msg(MALLOC_ERR_6002, LOG_ERR, pParam);
    exit(1);
  }
  
  pParam->trieNodeNum++;
  
  if (pElem == 0)
  {
    pNode->hashValue = pParam->wildcardHash;
    pNode->wildcardMin = pParam->prefixWildcardMin;
    pNode->wildcardMax = pParam->prefixWildcardMax;
  }
  else
  {
    pNode->hashValue = str2hash(pElem->pKey, pParam->prefixSketchSize,
                  pParam->prefixSketchSeed);
    pNode->wildcardMin = 0;
    pNode->wildcardMax = 0;
  }
  
  pNode->pWord = pElem;
  
  pNode->pParent = pParent;
  pNode->pChild = 0;
  
  if (pPrev != 0)
  {
    pNode->pNext = pPrev->pNext;
    pPrev->pNext = pNode;
  }
  else
  {
    pNode->pNext = pParent->pChild;
    pParent->pChild = pNode;
  }
  
  pNode->pIsEnd = 0;
  
  return pNode;
}

/* There is a potential support value overlapping problem. Though rare, because
 the order I select cluster candidates to aggregate is from small constants to
 big constants, it could still happen.
 
 I am thinking of using the count in {struct Elem} as a mid-way storage,
 therefore the count in {struct Cluster} remains the unchanged during the
 aggregate process. After the aggregate process is done for every cluster
 candidates, transfer the count in {struct Elem} to {struct Cluster}. 
 
 This solution has been implemented. */
static void aggregate_candidates(struct Parameters *pParam)
{
  int i;
  struct Cluster *ptr;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterFamily[i];
    while (ptr)
    {
      if (get_first_wildcard_location(ptr) >= 0)
      {
        aggregate_candidate(ptr, pParam);
      }
      ptr = ptr->pNext;
    }
  }
  
  /* After aggregation is done, assign each cluster candidates with the
   post-processed support value. ptr->pElem->count acts as a mid transfer. */
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterFamily[i];
    while (ptr)
    {
      ptr->count = ptr->pElem->count;
      
      ptr = ptr->pNext;
    }
  }
}

/* Find the first wildcard of a cluster candidates, counting from left to
 right. In other words, find the first constant, who has a wildcard. */
static int get_first_wildcard_location(struct Cluster *pCluster)
{
  int i;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    if (pCluster->fullWildcard[i * 2 + 1] != 0)
    {
      return i;
    }
  }
  
  if (pCluster->fullWildcard[1] != 0)
  {
    return 0;
  }
  
  return -1;
}

/* This function is called by function aggregate_candidates(). */
static void aggregate_candidate(struct Cluster *pCluster, 
        struct Parameters *pParam)
{
  struct TrieNode *pParent;
  int firstWildcardLoc;
  wordnumber_t hash;
  
  firstWildcardLoc = get_first_wildcard_location(pCluster);
  hash = 0;
  
  if (firstWildcardLoc)
  {
    hash = str2hash(pCluster->ppWord[firstWildcardLoc]->pKey,
            pParam->prefixSketchSize, pParam->prefixSketchSeed);
  }
  
  pParent = get_common_parent(pCluster);
  find_more_specific(pParent, pCluster, firstWildcardLoc, 0, 0, hash, pParam);
  
}

/* Find the common parent of a cluster candidate. From this node on, who is
 considered as the common parent, we will find all the child branches. Those
 branches have potential of being specified expressions of our cluster
 candidate, thus their support values can be aggregated to our cluster
 candidate's support value. */
static struct TrieNode *get_common_parent(struct Cluster *pCluster)
{
  struct TrieNode *ptr;
  int reverseDepth;
  int i;
  
  reverseDepth = get_first_wildcard_reverse_depth(pCluster);
  ptr = pCluster->pLastNode;
  
  for (i = 1; i <= reverseDepth; i++)
  {
    ptr = ptr->pParent;
  }
  
  /* ptr is the parent of the first wildcard node. */
  return ptr;
}

/* Find the nearest wildcard, counting from the lowest leaf towards root. */
static int get_first_wildcard_reverse_depth(struct Cluster *pCluster)
{
  int location;
  int i;
  int reverseDepth;
  
  location = get_first_wildcard_location(pCluster);
  
  if (location == -1)
  {
    return 0;
  }
  
  if (location == 0)
  {
    return 1;
  }
  
  reverseDepth = 0;
  
  for (i = location; i <= pCluster->constants; i++)
  {
    
    if (pCluster->fullWildcard[i * 2 + 1] != 0)
    {
      reverseDepth++;
    }
    reverseDepth++;
  }
  
  if (pCluster->fullWildcard[1] != 0)
  {
    reverseDepth++;
  }
  
  return reverseDepth;  
}

/* The function to find the more specific cluster candidates for a certain
 cluster candidate. */
static int find_more_specific(struct TrieNode *pParent, 
        struct Cluster *pCluster, int constant, int min, int max, 
        wordnumber_t hash, struct Parameters *pParam)
{
  struct TrieNode *ptr;
  wordnumber_t hashTmp;
  
  /* To find the 0st constant, means to deal with the tail of the cluster
   candidates. */
  if (constant == 0)
  {
    find_more_specific_tail(pParent, pCluster, min, max);
    return 0;
  }
  
  for (ptr = pParent->pChild; ptr; ptr = ptr->pNext)
  {
    if (ptr->wildcardMax == 0)
    {
      min += 1;
      max += 1;
    }
    else
    {
      min += ptr->wildcardMin;
      max += ptr->wildcardMax;
    }
    
    /* If the jump time is not enough to statisfy the minimum wildcard, jump
     down the tree once more, still looking for this constant. */
    if (min - 1 < pCluster->fullWildcard[constant * 2])
    {
      find_more_specific(ptr, pCluster, constant, min, max, hash, pParam);
      
      /* This node is done. Deal with its brothers. */
      if (ptr->wildcardMax == 0)
      {
        min -= 1;
        max -= 1;
      }
      else
      {
        min -= ptr->wildcardMin;
        max -= ptr->wildcardMax;
      }
      continue;
    }
    
    if (max - 1 > pCluster->fullWildcard[(constant * 2) + 1])
    {
      /* Jumped over the maximum limit. Not possible to be more specific
       cluster candidate anymore. */
      //break;
      
      if (ptr->wildcardMax == 0)
      {
        min -= 1;
        max -= 1;
      }
      else
      {
        min -= ptr->wildcardMin;
        max -= ptr->wildcardMax;
      }
      continue;
    }
    
    if (ptr->hashValue == hash &&
      (!strcmp(ptr->pWord->pKey, pCluster->ppWord[constant]->pKey)))
    {   //Found
      /* The constants are not all found, continue to look up next
       constant. */
      if (constant < pCluster->constants)
      {
        hashTmp = str2hash(pCluster->ppWord[constant + 1]->pKey,
                   pParam->prefixSketchSize,
                   pParam->prefixSketchSeed);
        find_more_specific(ptr, pCluster, constant + 1, 0, 0, hashTmp,
                   pParam);
        
        /* After coming back, continue to deal with brothers. */
        if (ptr->wildcardMax == 0)
        {
          min -= 1;
          max -= 1;
        }
        else
        {
          min -= ptr->wildcardMin;
          max -= ptr->wildcardMax;
        }
        continue;
      }
      /* If all the constants are found. There will be two cases to be
       considered:
       1. there is a wildcard in tail.(Tail means what is after the last
       constant).
       2. there is no wildcard in tail. */
      if (constant == pCluster->constants)
      {
        /* If there is no wildcard in tail, and if this node is a
         cluster candidate's end node, one result is found. We
         aggregate the support value. */
        if (pCluster->fullWildcard[1] == 0)
        {
          if (ptr->pIsEnd && (ptr->pIsEnd != pCluster))
          {
            //aggregate support
            //pCluster->count += ptr->pIsEnd->count;
            pCluster->pElem->count += ptr->pIsEnd->count;
          }
          
          /* Continue to deal with its brothers. */
          if (ptr->wildcardMax == 0)
          {
            min -= 1;
            max -= 1;
          }
          else
          {
            min -= ptr->wildcardMin;
            max -= ptr->wildcardMax;
          }
          continue;
        }
        else
        {
          /* If there is a wildcard in tail, continue. Note the third
           parameter is set to 0, which is different from normal
           cases, and will trigger function find_more_specific_tail().
           */
          if (pCluster->fullWildcard[0] == 0 && ptr->pIsEnd &&
            (ptr->pIsEnd != pCluster))
          {
            //aggregate support
            //pCluster->count += ptr->pIsEnd->count;
            pCluster->pElem->count += ptr->pIsEnd->count;
          }
          
          find_more_specific(ptr, pCluster, 0, 0, 0, hash, pParam);
          
          if (ptr->wildcardMax == 0)
          {
            min -= 1;
            max -= 1;
          }
          else
          {
            min -= ptr->wildcardMin;
            max -= ptr->wildcardMax;
          }
          continue;
        }
      }
      
    }
    else
    {
      find_more_specific(ptr, pCluster, constant, min, max, hash, pParam);
      
      if (ptr->wildcardMax == 0)
      {
        min -= 1;
        max -= 1;
      }
      else
      {
        min -= ptr->wildcardMin;
        max -= ptr->wildcardMax;
      }
      continue;
    }
  }
  
  return 0;
}

static int find_more_specific_tail(struct TrieNode *pParent, 
        struct Cluster*pCluster, int min, int max)
{
  struct TrieNode *ptr;
  
  for (ptr = pParent->pChild; ptr; ptr = ptr->pNext)
  {
    if (ptr->wildcardMax == 0)
    {
      min += 1;
      max += 1;
    }
    else
    {
      min += ptr->wildcardMin;
      max += ptr->wildcardMax;
    }
    
    if (min < pCluster->fullWildcard[0])
    {
      find_more_specific_tail(ptr, pCluster, min, max);
      if (ptr->wildcardMax == 0)
      {
        min -= 1;
        max -= 1;
      }
      else
      {
        min -= ptr->wildcardMin;
        max -= ptr->wildcardMax;
      }
      continue;
    }
    
    if (max > pCluster->fullWildcard[1])
    {
      /* Exceeds the legal jump range. Not possible to be a more specific
       cluster candidates any more. */
      //break;
      if (ptr->wildcardMax == 0)
      {
        min -= 1;
        max -= 1;
      }
      else
      {
        min -= ptr->wildcardMin;
        max -= ptr->wildcardMax;
      }
      
      continue;
    }
    
    if (ptr->pIsEnd && (ptr->pIsEnd != pCluster))
    {
      //aggregate support
      //pCluster->count += ptr->pIsEnd->count;
      pCluster->pElem->count += ptr->pIsEnd->count;
    }
    
    find_more_specific_tail(ptr, pCluster, min, max);
    if (ptr->wildcardMax == 0)
    {
      min -= 1;
      max -= 1;
    }
    else
    {
      min -= ptr->wildcardMin;
      max -= ptr->wildcardMax;
    }
    //continue;
  }
  
  return 0;
}