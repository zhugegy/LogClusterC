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
 * File:   clusters.c
 * 
 * Content: Functions related to Step3. Clusters.
 *
 * Created on November 30, 2016, 8:36 AM
 */

#include "common_header.h"
#include "clusters.h"

wordnumber_t step_3_find_clusters_from_candidates(struct Parameters *pParam)
{
  int clusterNum;
  struct Cluster *ptr, *pNext, *pPrev;
  int i;
  
  clusterNum = 0;
  
  for (i = 1; i <= pParam->biggestConstants; i++)
  {
    ptr = pParam->pClusterFamily[i];
    pPrev = 0;
    while (ptr)
    {
      if (ptr->count >= pParam->support)
      {
        clusterNum++;
        //print_cluster(ptr);
        pPrev = ptr;
        ptr = ptr->pNext;
      }
      else
      {
        /* Delete this cluster candidate. Only from pClusterFamily[],
         but not from cluster hash table. */
        if (pPrev)
        {
          pPrev->pNext = ptr->pNext;
        }
        else
        {
          pParam->pClusterFamily[i] = ptr->pNext;
        }
        pNext = ptr->pNext;
        free((void *) ptr->ppWord);
        free((void *) ptr->fullWildcard);
        free((void *) ptr);
        ptr = pNext;
      }
    }
  }
  
  return clusterNum;
}