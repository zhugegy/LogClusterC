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
 * File:   output.c
 * 
 * Content: Functions related to outputting messages (and logs).
 *
 * Created on November 29, 2016, 10:57 PM
 */

#include "common_header.h"
#include "output.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "utility.h"

static void print_clusters_default_config(struct Parameters *pParam);
static void print_clusters_constant_config(struct Parameters *pParam);

static void print_clusters_if_join_cluster_default_0(
  struct Parameters *pParam);
static void print_clusters_default_0(struct Parameters *pParam);

static void print_clusters_if_join_cluster_constant_1(
  struct Parameters *pParam);
static void print_clusters_constant_1(struct Parameters *pParam);

static void print_cluster(struct Cluster* pCluster);
static void print_cluster_with_token(struct ClusterWithToken *pClusterWithToken,
                struct Parameters *pParam);

/* Log message operator. It refines a message into timestamped format, and
 forwards it to user terminal. It also forwards the message to Syslog. */
void log_msg(char *message, int logLv, struct Parameters* pParam)
{
  time_t t;
  char *timestamp;
  
  t = time(0);
  timestamp = ctime(&t);
  timestamp[strlen(timestamp) - 1] = 0;
  fprintf(stderr, "%s: %s\n", timestamp, message);
  
  if (pParam->bSyslogFlag == 1)
  {
    syslog(logLv, "%s", message);
  }
}

void print_usage()
{
  fprintf(stderr, "\n");
  fprintf(stderr, VERSIONINFO);
  fprintf(stderr, "\n");
  fprintf(stderr, USAGEINFO);
}

void print_cluster_to_string(struct Cluster *pCluster, 
        struct Parameters *pParam)
{
  int i;
  //To avoid warning(returing local varialbe in stack), changed local variable
  //to outside variable(pParam->clusterDescription).
  //char clusterDescription[MAXLOGMSGLEN];
  char strTmp[MAXLOGMSGLEN];
  
  *pParam->clusterDescription = 0;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    if (pCluster->fullWildcard[i * 2 + 1])
    {
      sprintf(strTmp, "*{%d,%d} ", pCluster->fullWildcard[i * 2],
          pCluster->fullWildcard[i * 2 + 1]);
      strcat(pParam->clusterDescription, strTmp);
    }
    sprintf(strTmp, "%s ", pCluster->ppWord[i]->pKey);
    strcat(pParam->clusterDescription, strTmp);
  }
  
  if (pCluster->fullWildcard[1])
  {
    sprintf(strTmp, "*{%d,%d}", pCluster->fullWildcard[0],
        pCluster->fullWildcard[1]);
    strcat(pParam->clusterDescription, strTmp);
  }
  //return clusterDescription;
}

void step_3_print_clusters(struct Parameters *pParam)
{
  printf("\n");
  
  switch (pParam->outputMode)
  {
    case 0:
      //Default printing configuration. Clusters are sorted by support.
      print_clusters_default_config(pParam);
      break;
    case 1:
      /* Alternative printing configuration. Clusters are sorted by the number 
       of constants. */  
      print_clusters_constant_config(pParam);
      break;
    default:
      break;
  }
  
  printf("\n");
}

//clusters are arranged according to their support value
static void print_clusters_default_config(struct Parameters *pParam)
{
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  if (pParam->wordWeightThreshold)
  {
    print_clusters_if_join_cluster_default_0(pParam);
    
    str_format_int_grouped(digit, pParam->clusterNum -
                 pParam->joinedClusterInputNum +
                 pParam->joinedClusterOutputNum);
  }
  else
  {
    print_clusters_default_0(pParam);
    
    str_format_int_grouped(digit, pParam->clusterNum);
  }
  
  sprintf(logStr, "Total number of clusters: %s", digit);
  log_msg(logStr, LOG_INFO, pParam);
}

//clusters are arranged according to the number of constants (frequent words)
static void print_clusters_constant_config(struct Parameters *pParam)
{
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  
  if (pParam->wordWeightThreshold)
  {
    print_clusters_if_join_cluster_constant_1(pParam);
    
    str_format_int_grouped(digit, pParam->clusterNum -
                 pParam->joinedClusterInputNum +
                 pParam->joinedClusterOutputNum);
  }
  else
  {
    print_clusters_constant_1(pParam);
    
    str_format_int_grouped(digit, pParam->clusterNum);
  }
  
  sprintf(logStr, "Total number of clusters: %s", digit);
  log_msg(logStr, LOG_INFO, pParam);
  
}

/* When making changes to this function, don't forget to also change its
 brother function print_clusters_default_0(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static void print_clusters_if_join_cluster_default_0(
  struct Parameters *pParam)
{
  int i, j, k;
  struct Cluster *pCluster;
  struct ClusterWithToken *pClusterWithToken, *ptr;
  struct Elem **ppSortedArray;
  wordnumber_t toBeSortedNum;
  
  toBeSortedNum = (pParam->clusterNum - pParam->joinedClusterInputNum) +
  pParam->joinedClusterOutputNum;
  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) * 
          toBeSortedNum);
  if (!ppSortedArray)
  {
    log_msg(MALLOC_ERR_6020, LOG_ERR, pParam);
    exit(1);
  }
  
  j = 0;
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    pCluster = pParam->pClusterFamily[i];
    while (pCluster)
    {
      if (pCluster->bIsJoined == 0)
      {
        ppSortedArray[j] = pCluster->pElem;
        j++;
      }
      pCluster = pCluster->pNext;
    }
    
    pClusterWithToken = pParam->pClusterWithTokenFamily[i];
    while (pClusterWithToken)
    {
      ppSortedArray[j] = pClusterWithToken->pElem;
      j++;
      pClusterWithToken = pClusterWithToken->pNext;
    }
  }
  
  sort_elements(ppSortedArray, toBeSortedNum, pParam);
  
  for (k = 0; k < toBeSortedNum; k++)
  {
    /* For clusters in pClusterFamily[], only print those who were not
     marked as bIsJoined. Those who were joined, will be printed later, by
     accessing pClusterWithTokenFamily[]. */
    if (ppSortedArray[k]->pCluster->bIsJoined == 1)
    {
      ptr = (struct ClusterWithToken *) ppSortedArray[k]->pCluster;
      print_cluster_with_token(ptr, pParam);
    }
    else
    {
      print_cluster(ppSortedArray[k]->pCluster);
    }
  }
  
  free((void *) ppSortedArray);
}

//Outdated function
//void (O)_print_clusters_if_join_cluster_default_0(struct Parameters *pParam)
//{
//  int i, j, k, c, u, b;
//  struct Cluster *pCluster;
//  struct ClusterWithToken *pClusterWithToken;
//  struct Elem **ppSortedArray;
//  
//  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) *
//                      pParam->clusterNum);
//  if (!ppSortedArray)
//  {
//    log_msg(MALLOC_ERR_6019, LOG_ERR, pParam);
//    exit(1);
//  }
//  
//  j = 0;
//  for (i = 1; i <= pParam->biggestConstants; i++)
//  {
//    pCluster = pParam->pClusterFamily[i];
//    while (pCluster)
//    {
//      ppSortedArray[j] = pCluster->pElem;
//      j++;
//      pCluster = pCluster->pNext;
//    }
//  }
//  
//  sort_elements(ppSortedArray, pParam->clusterNum, pParam);
//  
//  if (pParam->clusterNum - pParam->joinedClusterInputNum)
//  {
//    printf(">>>>>>The %lu clusters that are not joined:\n\n",
//         pParam->clusterNum - pParam->joinedClusterInputNum);
//  }
//  
//  for (k = 0; k < pParam->clusterNum; k++)
//  {
//    /* For clusters in pClusterFamily[], only print those who were not
//     marked as bIsJoined. Those who were joined, will be printed later, by
//     accessing pClusterWithTokenFamily[]. */
//    if (ppSortedArray[k]->pCluster->bIsJoined == 0)
//    {
//      print_cluster(ppSortedArray[k]->pCluster);
//    }
//    
//  }
//  
//  free((void *) ppSortedArray);
//  
//  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) *
//                      pParam->joinedClusterOutputNum);
//  if (!ppSortedArray)
//  {
//    log_msg(MALLOC_ERR_6019, LOG_ERR, pParam);
//    exit(1);
//  }
//  
//  u = 0;
//  for (c = 1; c <= pParam->biggestConstants; c++)
//  {
//    pClusterWithToken = pParam->pClusterWithTokenFamily[c];
//    while (pClusterWithToken)
//    {
//      ppSortedArray[u] = pClusterWithToken->pElem;
//      u++;
//      pClusterWithToken = pClusterWithToken->pNext;
//    }
//  }
//  
//  sort_elements(ppSortedArray, pParam->joinedClusterOutputNum, pParam);
//  
//  if (pParam->joinedClusterOutputNum)
//  {
//    printf(">>>>>>The %lu joined clusters:\n\n",
//         pParam->joinedClusterOutputNum);
//  }
//  
//  for (b = 0; b < pParam->joinedClusterOutputNum; b++)
//  {
//    print_cluster_with_token((struct ClusterWithToken *)
//                 ppSortedArray[b]->pCluster, pParam);
//  }
//  
//  free((void *) ppSortedArray);
//}


/* When making changes to this function, don't forget to also change its
 brother function print_clusters_if_join_cluster_default_0(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static void print_clusters_default_0(struct Parameters *pParam)
{
  int i, j, k;
  struct Cluster *pCluster;
  struct Elem **ppSortedArray;
  
  ppSortedArray = (struct Elem **) malloc(sizeof(struct Elem *) *
                      pParam->clusterNum);
  if (!ppSortedArray)
  {
    log_msg(MALLOC_ERR_6018, LOG_ERR, pParam);
    exit(1);
  }
  
  j = 0;
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    pCluster = pParam->pClusterFamily[i];
    while (pCluster)
    {
      ppSortedArray[j] = pCluster->pElem;
      j++;
      pCluster = pCluster->pNext;
    }
  }
  
  sort_elements(ppSortedArray, pParam->clusterNum, pParam);
  
  for (k = 0; k < pParam->clusterNum; k++)
  {
    print_cluster(ppSortedArray[k]->pCluster);
  }
  
  free((void *) ppSortedArray);
}

/* When making changes to this function, don't forget to also change its
 brother function print_clusters_constant_1(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static void print_clusters_if_join_cluster_constant_1(
  struct Parameters *pParam)
{
  int i;
  struct Cluster *pCluster;
  struct ClusterWithToken *pClusterWithToken;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    /* For clusters in pClusterFamily[], only print those who were not
     marked as bIsJoined. Those who were joined, will be printed later, by
     accessing pClusterWithTokenFamily[]. */
    pCluster = pParam->pClusterFamily[i];
    while (pCluster)
    {
      if (pCluster->bIsJoined == 0)
      {
        print_cluster(pCluster);
        
      }
      pCluster = pCluster->pNext;
    }
    
    pClusterWithToken = pParam->pClusterWithTokenFamily[i];
    while (pClusterWithToken)
    {
      print_cluster_with_token(pClusterWithToken, pParam);
      pClusterWithToken = pClusterWithToken->pNext;
    }
    
  }
}

//outdated function
//void (O)_print_clusters_if_join_cluster_constant_1(struct Parameters *pParam)
//{
//  int i, j;
//  struct Cluster *pCluster;
//  struct ClusterWithToken *pClusterWithToken;
//  
//  if (pParam->clusterNum - pParam->joinedClusterInputNum)
//  {
//    printf(">>>>>>The %lu clusters that are not joined:\n\n",
//         pParam->clusterNum - pParam->joinedClusterInputNum);
//  }
//  
//  for (i = 1; i <= pParam->biggestConstants; i++)
//  {
//    /* For clusters in pClusterFamily[], only print those who were not
//     marked as bIsJoined. Those who were joined, will be printed later, by
//     accessing pClusterWithTokenFamily[]. */
//    pCluster = pParam->pClusterFamily[i];
//    while (pCluster)
//    {
//      if (pCluster->bIsJoined == 0)
//      {
//        print_cluster(pCluster);
//        
//      }
//      pCluster = pCluster->pNext;
//    }
//  }
//  
//  if (pParam->joinedClusterOutputNum)
//  {
//    printf(">>>>>>The %lu joined clusters:\n\n",
//         pParam->joinedClusterOutputNum);
//  }
//  
//  for (j = 1;  j <= pParam->biggestConstants; j++)
//  {
//    pClusterWithToken = pParam->pClusterWithTokenFamily[j];
//    while (pClusterWithToken)
//    {
//      print_cluster_with_token(pClusterWithToken, pParam);
//      pClusterWithToken = pClusterWithToken->pNext;
//    }
//  }
//}

/* When making changes to this function, don't forget to also change its
 brother function print_clusters_if_join_cluster_constant_1(). 
 For the sake of computing performance, sorry for this inconvenience. It will
 be fixed with better solution in the following updates. */
static void print_clusters_constant_1(struct Parameters *pParam)
{
  int i;
  struct Cluster *pCluster;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    pCluster = pParam->pClusterFamily[i];
    while (pCluster)
    {
      print_cluster(pCluster);
      pCluster = pCluster->pNext;
    }
  }
}

static void print_cluster(struct Cluster* pCluster)
{
  char digit[MAXDIGITBIT];
  int i;
  
  for (i = 1; i <= pCluster->constants; i++)
  {
    if (pCluster->fullWildcard[i * 2 + 1])
    {
      printf("*{%d,%d} ", pCluster->fullWildcard[i * 2],
           pCluster->fullWildcard[i * 2 + 1]);
    }
    printf("%s ", pCluster->ppWord[i]->pKey);
  }
  
  if (pCluster->fullWildcard[1])
  {
    printf("*{%d,%d}", pCluster->fullWildcard[0],
         pCluster->fullWildcard[1]);
  }
  
  printf("\n");
  
  str_format_int_grouped(digit, pCluster->count);
  printf("Support : %s\n\n", digit);
}

static void print_cluster_with_token(struct ClusterWithToken *pClusterWithToken,
                struct Parameters *pParam)
{
  char digit[MAXDIGITBIT];
  struct Token *pToken;
  int i;
  
  for (i = 1; i <= pClusterWithToken->constants; i++)
  {
    if (pClusterWithToken->fullWildcard[i * 2 + 1])
    {
      printf("*{%d,%d} ", pClusterWithToken->fullWildcard[i * 2],
           pClusterWithToken->fullWildcard[i * 2 + 1]);
    }
    
    if (pClusterWithToken->ppToken[i] != 0)
    {
      if (pParam->bDetailedTokenFlag == 0)
      {
        /* This solution will not mark a token, if it is the only word.
         */
        if (pClusterWithToken->ppToken[i]->pNext != 0)
        {
          printf("(");
          pToken = pClusterWithToken->ppToken[i];
          while (pToken)
          {
            printf("%s", pToken->pWord->pKey);
            if (pToken->pNext)
            {
              printf("|");
            }
            pToken = pToken->pNext;
          }
          printf(") ");
        }
        else
        {
          printf("%s ", pClusterWithToken->ppToken[i]->pWord->pKey);
        }
      }
      else
      {
        /* This solution marks a token with (), no matter how many words
         it contains. */
        printf("(");
        pToken = pClusterWithToken->ppToken[i];
        while (pToken)
        {
          printf("%s", pToken->pWord->pKey);
          if (pToken->pNext)
          {
            printf("|");
          }
          pToken = pToken->pNext;
        }
        printf(") ");
      }
      
    }
    else
    {
      printf("%s ", pClusterWithToken->ppWord[i]->pKey);
    }
  }
  
  if (pClusterWithToken->fullWildcard[1])
  {
    printf("*{%d,%d}", pClusterWithToken->fullWildcard[0],
         pClusterWithToken->fullWildcard[1]);
  }
  
  printf("\n");
  
  str_format_int_grouped(digit, pClusterWithToken->count);
  printf("Support : %s\n\n", digit);
}