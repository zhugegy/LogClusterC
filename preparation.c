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
 * File:   preparation.c
 * 
 * Content: Functions related to Step0 Preparation.
 *
 * Created on November 29, 2016, 10:51 PM
 */

#include "common_header.h"
#include "preparation.h"

#include <getopt.h>    /* for get_opt_long() */
#include <glob.h>      /* for glob() */
#include <string.h>    /* for strcmp(), strcpy(), etc. */
#include <regex.h>     /* for regcomp() and regexec() */

#include "output.h"
#include "free_resource.h"
#include "utility.h"

static void glob_filenames(char *pPattern, struct Parameters *pParam);
static void build_input_file_chain(char *pFilename, struct Parameters *pParam);
static void build_template_chain(char *opt, struct Parameters *pParam);
static int change_syslog_facility_number(struct Parameters *pParam);
static int validate_parameters_template(struct Parameters *pParam);

/* Initialization of parameters */
int step_0_init_input_parameters(struct Parameters *pParam)
{
  int i;//
  char *defSyslogFacility = DEF_SYSLOG_FACILITY;
  
  pParam->support = 0;
  pParam->pctSupport = 0;
  pParam->pInputFiles = 0;
  pParam->initSeed = DEF_INIT_SEED;
  pParam->wordTableSize = DEF_WORD_TABLE_SIZE;
  pParam->bSyslogFlag = 0;
  pParam->bDetailedTokenFlag = 0;
  
  pParam->pSyslogFacility = (char *) malloc(strlen(defSyslogFacility) + 1);
  if (!pParam->pSyslogFacility)
  {
    log_msg(MALLOC_ERR_6001, LOG_ERR, pParam);
    exit(1);
  }
  strcpy(pParam->pSyslogFacility, defSyslogFacility);
  
  pParam->pDelim = 0;
  pParam->byteOffset = 0;
  pParam->pFilter = 0;
  pParam->pTemplate = 0;
  pParam->wordSketchSize = 0;
  pParam->clusterSketchSize = 0;
  pParam->bAggrsupFlag = 0;
  pParam->wordWeightThreshold = 0;
  pParam->wordWeightFunction = 1;
  pParam->pOutlier = 0;
  pParam->debug = 0;
  pParam->outputMode = 0;
  
  pParam->syslogThreshold = DEF_SYSLOG_THRESHOLD;
  pParam->syslogFacilityNum = LOG_LOCAL2;
  pParam->wordTableSeed = 0;
  pParam->ppWordTable = 0;
  pParam->pWordSketch = 0;
  pParam->wordSketchSeed = 0;
  pParam->linecount = 0;
  pParam->dataPassTimes = 0;
  pParam->totalLineNum = 0;
  *pParam->totalLineNumDigit = 0;
  pParam->timeStorage = 0;
  pParam->freWordNum = 0;
  pParam->clusterNum = 0;
  pParam->clusterCandiNum = 0;
  pParam->pClusterSketch = 0;
  pParam->clusterSketchSeed = 0;
  pParam->clusterTableSize = 0;
  pParam->ppClusterTable = 0;
  pParam->clusterTableSeed = 0;
  pParam->biggestConstants = 0;
  pParam->wordDepMatrix = 0;
  pParam->wordDepMatrixBreadth = 0;
  pParam->trieNodeNum = 0;
  
  /* struct Cluster *clusterFamily[MAXWORDS + 1]; */
  for (i = 0; i < MAXWORDS + 1; i++)
  {
    pParam->pClusterFamily[i] = 0;
  }
  
  /* The initialzition of regex_t delim_regex is integrated to function
   validate_parameters(). */
  
  /* The initialzition of regex_t filter_regex is integrated to function
   validate_parameters(). */
  
  pParam->wildcardHash = 0;
  pParam->prefixSketchSize = 0;
  pParam->prefixSketchSeed = 0;
  pParam->prefixWildcardMin = 0;
  pParam->prefixWildcardMax = 0;
  pParam->pPrefixRoot = 0;
  pParam->pPrefixRet = 0;
  
  /* If "token" is in frequent words, another random string that is not in
   frequent words will replace "token". */
  strcpy(pParam->token, "token");
  
  for (i = 0; i <= MAXWORDS; i++)
  {
    pParam->tokenMarker[i] = 0;
  }
  
  pParam->joinedClusterInputNum = 0;
  pParam->joinedClusterOutputNum = 0;
  
  for (i = 0; i < MAXWORDS + 1; i++)
  {
    pParam->pClusterWithTokenFamily[i] = 0;
  }
  
  for (i = 0; i < MAXWORDS + 1; i++)
  {
    pParam->wordNumStr[i] = 0;
  }
  
  pParam->pCurrentCluster = 0;
  
  *pParam->clusterDescription = 0;
  
  /* The initialzition of regex_t wfilter_regex and wsearch_regex is 
   integrated to function validate_parameters(). */
  pParam->pWordFilter = 0;
  pParam->pWordSearch = 0;
  pParam->pWordReplace = 0;
  *pParam->tmpStr = 0;
  
  return 1;
}

int step_0_parse_options(int argc, char **argv, struct Parameters *pParam)
{
  extern char *optarg;
  extern int optind;
  int c;
  char logStr[MAXLOGMSGLEN];
  
  static struct option long_options[] =
  {
    {"aggrsup",   no_argument,     0,   'a'},
    {"byteoffset",  required_argument, 0,   'b'},
    {"csize",     required_argument, 0,   'c'},
    {"debug",     optional_argument, 0,  1007},
    {"detailtoken", no_argument,     0,  1012},
    {"help",    no_argument,     0,   'h'},
    {"initseed",  required_argument, 0,   'i'},
    {"lfilter",   required_argument, 0,   'f'},
    {"input",     required_argument, 0,  1001},
    {"outliers",  required_argument, 0,   'o'},
    {"outputmode",  optional_argument, 0,  1011},
    {"rsupport",  required_argument, 0,  1005},
    {"separator",   required_argument, 0,   'd'},
    {"support",   required_argument, 0,   's'},
    {"syslog",    optional_argument, 0,  1002},
    {"template",  required_argument, 0,   't'},
    {"version",   no_argument,     0,  1006},
    {"weightf",   required_argument, 0,  1004},
    {"wfilter",   required_argument, 0,  1008},
    {"wreplace",  required_argument, 0,  1010},
    {"wsearch",   required_argument, 0,  1009},
    {"wsize",     required_argument, 0,   'v'},
    {"wtablesize",  required_argument, 0,   'w'},
    {"wweight",   required_argument, 0,  1003},
    {0, 0, 0, 0}
  };
  /* getopt_long stores the option index here. */
  int optionIndex = 0;
  
  while ((c = getopt_long(argc, argv, "s:i:w:d:b:f:t:v:c:ao:h",
              long_options, &optionIndex)) != -1)
  {
    switch (c)
    {
      case 0:
        break;
      case 'a':
        pParam->bAggrsupFlag = 1;
        break;
      case 's':
        if (optarg[strlen(optarg) - 1] == '%')
        {
          pParam->pctSupport = atof(optarg);
        }
        else
        {
          pParam->support = labs(atol(optarg));
        }
        break;
      case 'i':
        pParam->initSeed = abs(atoi(optarg));
        break;
      case 'w':
        pParam->wordTableSize = labs(atol(optarg));
        break;
      case 1001:
        glob_filenames(optarg, pParam);
        break;
      case 1002:
        pParam->bSyslogFlag = 1;
        if (optarg)
        {
          free_syslog_facility(pParam);
          pParam->pSyslogFacility = (char *) malloc(strlen(optarg) + 1);
          if (!pParam->pSyslogFacility)
          {
            log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
            exit(1);
          }
          strcpy(pParam->pSyslogFacility, optarg);
          string_lowercase(pParam->pSyslogFacility);
        }
        break;
      case 'd':
        pParam->pDelim = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pDelim)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pDelim, optarg);
        break;
      case 'b':
        pParam->byteOffset = atoi(optarg);
        break;
      case 'f':
        pParam->pFilter = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pFilter)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pFilter, optarg);
        break;
      case 't':
        build_template_chain(optarg, pParam);
        break;
      case 'v':
        pParam->wordSketchSize = labs(atol(optarg));
        break;
      case 'c':
        pParam->clusterSketchSize = labs(atol(optarg));
        break;
      case 1003:
        pParam->wordWeightThreshold = atof(optarg);
        break;
      case 1004:
        pParam->wordWeightFunction = atoi(optarg);
        break;
      case 1005:
        pParam->pctSupport = atof(optarg);
        break;
      case 'o':
        pParam->pOutlier = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pOutlier)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pOutlier, optarg);
        break;
      case 1006:
        printf("%s", VERSIONINFO);
        printf("\n");
        exit(0);
        break;
      case 'h':
        printf("%s", USAGEINFO);
        printf("%s", HELPINFO);
        printf("\n");
        exit(0);
        break;
      case 1007:
        pParam->debug = 1;
        if (optarg)
        {
          pParam->debug = atoi(optarg);
        }
        break;
      case 1008:
        pParam->pWordFilter = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pWordFilter)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pWordFilter, optarg);
        break;
      case 1009:
        pParam->pWordSearch = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pWordSearch)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pWordSearch, optarg);
        break;
      case 1010:
        pParam->pWordReplace = (char *) malloc(strlen(optarg) + 1);
        if (!pParam->pWordReplace)
        {
          log_msg(MALLOC_ERR_6006, LOG_ERR, pParam);
          exit(1);
        }
        strcpy(pParam->pWordReplace, optarg);
        break;
      case 1011:
        pParam->outputMode = 1;
        if (optarg)
        {
          pParam->outputMode = atoi(optarg);
        }
        break;
      case 1012:
        pParam->bDetailedTokenFlag = 1;
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;
      default:
        abort ();
    }
  }
  
  if (optind < argc)
  {
    //printf ("non-option ARGV-elements: ");
    while (optind < argc)
    {
      strcat(logStr, argv[optind++]);
      strcat(logStr, " ");
    }
    sprintf(logStr, "Non-option elements: %s.", logStr);
    log_msg(logStr, LOG_ERR, pParam);
    return 0;
  }
  
  return 1;
}

int step_0_validate_parameters(struct Parameters *pParam)
{
  char *defSyslogFacility = DEF_SYSLOG_FACILITY;
  
  if (pParam->support <= 0 && pParam->pctSupport <= 0)
  {
    log_msg("'-s', '--support' or '--rsupport' option requires a positive"
        "number as parameter", LOG_ERR, pParam);
    return 0;
  }
  
  if (!pParam->pInputFiles)
  {
    log_msg("No input files specified", LOG_ERR, pParam);
    return 0;
  }
  
  //Comparison of unsigned expression < 0 is always false.
  if (pParam->initSeed <= 0)
  {
    log_msg("'-i' or '--initseed' option requires a positive number or "
        "zero as parameter", LOG_ERR, pParam);
    return 0;
  }
  
  if (pParam->wordTableSize <= 0)
  {
    log_msg("'-w' or '--wtablesize' option requires a positive number as "
        "parameter", LOG_ERR, pParam);
    return 0;
  }
  
  if (strcmp(pParam->pSyslogFacility, defSyslogFacility))
  {
    //tune facility
    if(!change_syslog_facility_number(pParam))
    {
      return 0;
    }
  }
  
  if (pParam->pDelim)
  {
    if (regcomp(&pParam->delim_regex, pParam->pDelim, REG_EXTENDED))
    {
      log_msg("Bad regular expression given with '-d' or '--separator' "
          "option", LOG_ERR, pParam);
      return 0;
    }
  }
  else
  {
    regcomp(&pParam->delim_regex, DEF_WORD_DELM, REG_EXTENDED);
  }
  
  if (pParam->byteOffset < 0)
  {
    log_msg("'-b' or '--byteoffset' option requires a positive number as "
        "parameter", LOG_ERR, pParam);
    return 0;
  }
  
  if (pParam->pFilter && regcomp(&pParam->filter_regex, pParam->pFilter,
                   REG_EXTENDED))
  {
    log_msg("Bad regular expression given with '-f' or '--lfilter' option",
        LOG_ERR, pParam);
    return 0;
  }
  
  if (pParam->pWordFilter)
  {
    if (!pParam->pWordSearch || !pParam->pWordReplace)
    {
      log_msg("If you set '--wfilter' option, '--wsearch' and "
          "'--wreplace' must be set as well", LOG_ERR, pParam);
      return 0;
    }
  }
  
  if (pParam->pWordFilter && regcomp(&pParam->wfilter_regex,
                     pParam->pWordFilter, REG_EXTENDED))
  {
    log_msg("Bad regular expression given with '--wfilter' option",
        LOG_ERR, pParam);
    return 0;
  }
  
  if (pParam->pWordSearch && regcomp(&pParam->wsearch_regex,
                     pParam->pWordSearch, REG_EXTENDED))
  {
    log_msg("Bad regular expression given with '--wsearch' option",
        LOG_ERR, pParam);
    return 0;
  }
  
  if (!validate_parameters_template(pParam))
  {
    return 0;
  }
  
  //Comparison of unsigned expression < 0 is always false
  //if (pParam->wordSketchSize < 0)
  //{
  //  log_msg("'-v' or '--wsize' option requires a positive number as "
  //"parameter", LOG_ERR, pParam);
  //  return 0;
  //}
  
  //if (pParam->clusterSketchSize < 0)
  //{
  //  log_msg("'-c' or '--csize' option requires a positive number as "
  //"parameter", LOG_ERR, pParam);
  //  return 0;
  //}
  
  if (pParam->wordWeightThreshold < 0 || pParam->wordWeightThreshold > 1)
  {
    log_msg("'--wweight' option requires a valid number: 0<number<=1",
        LOG_ERR, pParam);
    return 0;
  }
  
  if (pParam->wordWeightFunction != 1 && pParam->wordWeightFunction != 2)
  {
    log_msg("'--weightf' option requires a valid number: 1 or 2", LOG_ERR,
        pParam);
    return 0;
  }
  
  if (pParam->debug != 0 && pParam->debug != 1 && pParam->debug != 2 &&
    pParam->debug != 3)
  {
    log_msg("'--debug' option requires a valid number: 1, 2 or 3", LOG_ERR,
        pParam);
    return 0;
  }
  
  if (pParam->outputMode != 0 && pParam->outputMode != 1)
  {
    log_msg("'--outputMode' option requires a valid number: 1", LOG_ERR,
        pParam);
    return 0;
  }
  
  if (pParam->clusterSketchSize && pParam->bAggrsupFlag)
  {
    log_msg("'--csize' option can not be used together with '--aggrsup' "
        "option", LOG_ERR, pParam);
    return 0;
  }
  
  return 1;
}

void step_0_generate_seeds(struct Parameters *pParam)
{
  pParam->wordTableSeed = rand();
  pParam->wordSketchSeed = rand();
  pParam->clusterSketchSeed =rand();
  pParam->clusterTableSeed = rand();
  pParam->prefixSketchSeed =rand();
}

int step_0_cal_total_pass_over_data_set_times(struct Parameters *pParam)
{
  int times;
  
  /* Build vocabulary, find cluster candidates. */
  times = 2;
  
  if (pParam->wordSketchSize) { times++; }
  if (pParam->clusterSketchSize) { times++; }
  if (pParam->pOutlier) { times++; }
  
  return times;
}


/* File path wildcard supporting. */
static void glob_filenames(char *pPattern, struct Parameters *pParam)
{
  glob_t globResults;
  char **ppFileNameVector;
  int i;
  
  glob(pPattern, GLOB_NOCHECK, 0, &globResults);
  
  ppFileNameVector = globResults.gl_pathv;
  
  for (i = 0; i < globResults.gl_pathc; i++)
  {
    build_input_file_chain(ppFileNameVector[i], pParam);
  }
  
  globfree(&globResults);
}

static void build_input_file_chain(char *pFilename, struct Parameters *pParam)
{
  struct InputFile *ptr;
  char logStr[MAXLOGMSGLEN];
  
  if (!pParam->pInputFiles)
  {
    pParam->pInputFiles = (struct InputFile *)
    malloc(sizeof(struct InputFile));
    if (!pParam->pInputFiles)
    {
      log_msg(MALLOC_ERR_6004, LOG_ERR, pParam);
      exit(1);
    }
    pParam->pInputFiles->pName = (char *) malloc(strlen(pFilename) + 1);
    if (!pParam->pInputFiles->pName)
    {
      log_msg(MALLOC_ERR_6004, LOG_ERR, pParam);
      exit(1);
    }
    strcpy(pParam->pInputFiles->pName, pFilename);
    pParam->pInputFiles->lineNumber = 0;
    pParam->pInputFiles->pNext = 0;
  }
  else
  {
    for (ptr = pParam->pInputFiles; ptr->pNext; ptr = ptr->pNext);
    ptr->pNext = (struct InputFile *) malloc(sizeof(struct InputFile));
    if (!ptr->pNext)
    {
      log_msg(MALLOC_ERR_6004, LOG_ERR, pParam);
      exit(1);
    }
    ptr = ptr->pNext;
    ptr->pName = (char *) malloc(strlen(pFilename) + 1);
    if (!ptr->pName)
    {
      log_msg(MALLOC_ERR_6004, LOG_ERR, pParam);
      exit(1);
    }
    strcpy(ptr->pName, pFilename);
    ptr->lineNumber = 0;
    ptr->pNext = 0;
  }
  
  sprintf(logStr, "File %s is added", pFilename);
  log_msg(logStr, LOG_INFO, pParam);
}

static void build_template_chain(char *opt, struct Parameters *pParam)
{
  static struct TemplElem *ptr = 0;
  int i, start, len;
  char *addr;
  
  i = 0;
  while (opt[i])
  {
    if (pParam->pTemplate)
    {
      ptr->pNext = (struct TemplElem *) malloc(sizeof(struct TemplElem));
      if (!ptr->pNext)
      {
        log_msg(MALLOC_ERR_6005, LOG_ERR, pParam);
        exit(1);
      }
      ptr = ptr->pNext;
    }
    else
    {
      pParam->pTemplate = (struct TemplElem *) malloc(sizeof(struct TemplElem));
      if (!pParam->pTemplate)
      {
        log_msg(MALLOC_ERR_6005, LOG_ERR, pParam);
        exit(1);
      }
      ptr = pParam->pTemplate;
    }
    
    if (opt[i] != BACKREFCHAR)
    {
      start = i;
      while (opt[i] && opt[i] != BACKREFCHAR)
      {
        i++;
      }
      len = i - start;
      ptr->pStr = (char *) malloc(len + 1);
      if (!ptr->pStr)
      {
        log_msg(MALLOC_ERR_6005, LOG_ERR, pParam);
        exit(1);
      }
      strncpy(ptr->pStr, opt + start, len);
      ptr->pStr[len] = 0;
      ptr->data = len;
    }
    else
    {
      ptr->pStr = 0;
      ptr->data = (int) strtol(opt + i + 1, &addr, 10);
      i = (int) (addr - opt);
    }
    
    ptr->pNext = 0;
  }
}

static int change_syslog_facility_number(struct Parameters *pParam)
{
  int i;
  
  for (i = 0; i < ARR_SIZE(pSyslogFacilityList); i++)
  {
    if (!strcmp(pParam->pSyslogFacility, pSyslogFacilityList[i]))
    {
      switch (i)
      {
        case 0:
          pParam->syslogFacilityNum = LOG_KERN;
          break;
        case 1:
          pParam->syslogFacilityNum = LOG_USER;
          break;
        case 2:
          pParam->syslogFacilityNum = LOG_MAIL;
          break;
        case 3:
          pParam->syslogFacilityNum = LOG_DAEMON;
          break;
        case 4:
          pParam->syslogFacilityNum = LOG_AUTH;
          break;
        case 5:
          pParam->syslogFacilityNum = LOG_SYSLOG;
          break;
        case 6:
          pParam->syslogFacilityNum = LOG_LPR;
          break;
        case 7:
          pParam->syslogFacilityNum = LOG_NEWS;
          break;
        case 8:
          pParam->syslogFacilityNum = LOG_UUCP;
          break;
        case 9:
          pParam->syslogFacilityNum = LOG_CRON;
          break;
        case 10:
          pParam->syslogFacilityNum = LOG_AUTHPRIV;
          break;
        case 11:
          pParam->syslogFacilityNum = LOG_FTP;
          break;
        case 12:
        case 13:
        case 14:
        case 15:
          pParam->syslogFacilityNum = LOG_LOCAL2;
          break;
        case 16:
          pParam->syslogFacilityNum = LOG_LOCAL0;
          break;
        case 17:
          pParam->syslogFacilityNum = LOG_LOCAL1;
          break;
        case 18:
          pParam->syslogFacilityNum = LOG_LOCAL2;
          break;
        case 19:
          pParam->syslogFacilityNum = LOG_LOCAL3;
          break;
        case 20:
          pParam->syslogFacilityNum = LOG_LOCAL4;
          break;
        case 21:
          pParam->syslogFacilityNum = LOG_LOCAL5;
          break;
        case 22:
          pParam->syslogFacilityNum = LOG_LOCAL6;
          break;
        case 23:
          pParam->syslogFacilityNum = LOG_LOCAL7;
          break;
        default:
          pParam->syslogFacilityNum = LOG_LOCAL2;
          break;
      }
      break;
    }
    else
    {
      if (i == ARR_SIZE(pSyslogFacilityList) - 1)
      {
        log_msg("'--syslog' option requires a legal string as "
            "parameter, e.g. \"local2\".", LOG_ERR, pParam);
        return 0;
      }
    }
  }
  
  return 1;
}

static int validate_parameters_template(struct Parameters *pParam)
{
  struct TemplElem *ptr;
  char logStr[MAXLOGMSGLEN];
  
  for (ptr = pParam->pTemplate; ptr; ptr = ptr->pNext)
  {
    if (!ptr->pStr && (ptr->data < 0 || ptr->data > MAXPARANEXPR -1))
    {
      sprintf(logStr, "'-t' or '--template' option requires"
          "backreference variables to be in range $0...$%d",
          MAXPARANEXPR - 1);
      log_msg(logStr, LOG_ERR, pParam);
      return 0;
    }
  }
  return 1;
}