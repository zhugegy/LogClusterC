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
 * File:   hash_table_processing.c
 * 
 * Content: Functions related to hash table processing. 
 *
 * Created on November 30, 2016, 4:30 AM
 */

#include "common_header.h"
#include "hash_table_processing.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "utility.h"
#include "output.h"

struct Elem *add_elem(char *pKey, struct Elem **ppTable, tableindex_t tablesize, 
        tableindex_t seed, struct Parameters *pParam)
{
  tableindex_t hash;
  struct Elem *ptr, *pPrev;
  
  hash = str2hash(pKey, tablesize, seed);
  
  if (ppTable[hash])
  {
    pPrev = 0;
    ptr = ppTable[hash];
    
    while (ptr)
    {
      if (!strcmp(pKey, ptr->pKey))
      {
        break;
      }
      pPrev = ptr;
      ptr = ptr->pNext;
    }
    
    if (ptr)
    {
      ptr->count++;
      
      if (pPrev)
      {
        pPrev->pNext = ptr->pNext;
        ptr->pNext = ppTable[hash];
        ppTable[hash] = ptr;
      }
      
    }
    else
    {
      ptr = (struct Elem *) malloc(sizeof(struct Elem));
      if (!ptr)
      {
        log_msg(MALLOC_ERR_6007, LOG_ERR, pParam);
        exit(1);
      }
      
      ptr->pKey = (char *) malloc(strlen(pKey) + 1);
      if (!ptr->pKey)
      {
        log_msg(MALLOC_ERR_6007, LOG_ERR, pParam);
        exit(1);
      }
      
      strcpy(ptr->pKey, pKey);
      ptr->count = 1;
      ptr->pNext = ppTable[hash];
      
      ppTable[hash] = ptr;
      
    }
  }
  else
  {
    ptr = (struct Elem *) malloc(sizeof(struct Elem));
    if (!ptr)
    {
      log_msg(MALLOC_ERR_6007, LOG_ERR, pParam);
      exit(1);
    }
    
    ptr->pKey = (char *) malloc(strlen(pKey) + 1);
    if (!ptr->pKey)
    {
      log_msg(MALLOC_ERR_6007, LOG_ERR, pParam);
      exit(1);
    }
    
    strcpy(ptr->pKey, pKey);
    ptr->count = 1;
    ptr->pNext = 0;
    
    ppTable[hash] = ptr;
  }
  
  return ptr;
}

struct Elem *find_elem(char *key, struct Elem **table, tableindex_t tablesize,
             tableindex_t seed)
{
  tableindex_t hash;
  struct Elem *ptr, *pPrev;
  
  pPrev = 0;
  hash = str2hash(key, tablesize, seed);
  
  for (ptr = table[hash]; ptr; ptr = ptr->pNext)
  {
    if (!strcmp(key, ptr->pKey))
    {
      break;
    }
    pPrev = ptr;
  }
  
  /* After success finding, Move-To-Front */
  if (ptr && pPrev)
  {
    pPrev->pNext = ptr->pNext;
    ptr->pNext = table[hash];
    table[hash] = ptr;
  }
  
  return ptr;
}
