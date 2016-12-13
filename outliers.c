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
 * File:   outliers.c
 * 
 * Content: Functions related to Step4. Outliers.
 *
 * Created on November 30, 2016, 10:48 PM
 */

#include "common_header.h"
#include "outliers.h"

#include <string.h>    /* for strcmp(), strcpy(), etc. */

#include "output.h"
#include "line_processing.h"
#include "hash_table_processing.h"

wordnumber_t step_4_find_outliers(struct Parameters *pParam)
{
  FILE *pOutliers;
  FILE *pFile;
  struct InputFile *pFilePtr;
  char logStr[MAXLOGMSGLEN];
  char line[MAXLINELEN];
  char key[MAXKEYLEN];
  char words[MAXWORDS][MAXWORDLEN];
  int len, wordcount, i;
  struct Elem *pWord, *pElem;
  wordnumber_t outlierNum;
  
  outlierNum = 0;
  
  if (!(pOutliers = fopen(pParam->pOutlier, "w")))
  {
    sprintf(logStr, "Can't open outliers file %s", pParam->pOutlier);
    log_msg(logStr, LOG_ERR, pParam);
    exit(1);
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
        }
      }
      
      if (*key == 0 && wordcount)
      {
        fprintf(pOutliers, "%s\n", line);
        outlierNum++;
        continue;
      }
      
      pElem = find_elem(key, pParam->ppClusterTable, pParam->clusterTableSize,
                pParam->clusterTableSeed);
      
      if (!pElem || (pElem->count < pParam->support))
      {
        fprintf(pOutliers, "%s\n", line);
        outlierNum++;
      }
    }
  }
  
  return outlierNum;
}