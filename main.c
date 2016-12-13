
/*
 LogClusterC version 0.04
 Copyright (C) 2016 Zhuge Chen, Risto Vaarandi and Mauno Pihelgas
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 LogCluster is a density-based data clustering algorithm for event logs,
 introduced by Risto Vaarandi and Mauno Pihelgas in 2015.
 
 A detialed discussion of the LogCluster algorithm can be found in the paper
 ( http://ristov.github.io/publications/cnsm15-logcluster-web.pdf ) published at
 CNSM 2015.
 */

#include "common_header.h"

#include <syslog.h>    /* for syslog() */

#include "preparation.h"
#include "frequent_words.h"
#include "cluster_candidates.h"
#include "clusters.h"
#include "outliers.h"
#include "aggregate_supports_heuristic.h"
#include "join_clusters_heuristic.h"
#include "output.h"
#include "free_resource.h"
#include "utility.h"

int main(int argc, char **argv)
{
  struct Parameters param;
  char logStr[MAXLOGMSGLEN];
  char digit[MAXDIGITBIT];
  wordnumber_t totalWordNum, outlierNum;
  
  /* ######## #### ## Step0 Preparation ## #### ######## */
  
  /* Step0.A Initialize parameters */
  if (!step_0_init_input_parameters(&param))
  {
    log_msg("Parameter initialization failed.", LOG_ERR, &param);
    exit(1);
  }
  
  /* Step0.B Parse command line options */
  if (!step_0_parse_options(argc, argv, &param))
  {
    log_msg("Option parse failed.", LOG_ERR, &param);
    print_usage();
    exit(1);
  }
  
  /* Step0.C Check validation of parameters */
  /* Some parameters were changed by command line. Check their validation. */
  if (!step_0_validate_parameters(&param))
  {
    log_msg("Parameters validation failed.", LOG_ERR, &param);
    print_usage();
    exit(1);
  }
  
  /* Step0.D Set syslog utility */
  /* Tag: Optional */
  if (param.bSyslogFlag == 1)
  {
    setlogmask(LOG_UPTO (param.syslogThreshold));
    openlog("logclusterc", LOG_CONS | LOG_PID | LOG_NDELAY,
        param.syslogFacilityNum);
  }
  
  /* Step0.E Generate seeds */
  /* Seeds are used to construct hash tables. */
  srand(param.initSeed);
  step_0_generate_seeds(&param);
  
  /* Step0.F Get times of pass over the data set */
  param.dataPassTimes = step_0_cal_total_pass_over_data_set_times(&param);
  
  /* Step0.G All is ready. Do the work. */
  log_msg("Starting...", LOG_NOTICE, &param);
  
  /* ######## #### ## Step1 Frequent Words ## #### ######## */
  
  /*Step1.A Create word sketch*/
  /*Tag: Optional, One pass over the data set*/
  /*Very useful in mining process of large log files, e.g. more than 1GB. It
   significantly optimizes memory consumption.*/
  if (param.wordSketchSize)
  {
    step_1_create_word_sketch(&param);
    param.totalLineNum = param.linecount * param.dataPassTimes;
    str_format_int_grouped(param.totalLineNumDigit, param.totalLineNum);
  }
  
  /*Step1.B Create vocabulary*/
  /*Tag: One pass over the data set*/
  totalWordNum = step_1_create_vocabulary(&param);
  if (!param.totalLineNum)
  {
    param.totalLineNum = param.linecount * param.dataPassTimes;
    str_format_int_grouped(param.totalLineNumDigit, param.totalLineNum);
  }
  
  /*Step1.C Finding frequent words*/
  /*It also santizes word table, moving words under support out of table.*/
  log_msg("Finding frequent words from vocabulary...", LOG_NOTICE, &param);
  
  param.freWordNum = step_1_find_frequent_words(&param, totalWordNum);
  
  /*Step1.D Debug_1 mode: print frequent words*/
  /*Tag: Optional*/
  if (param.debug == 1)
  {
    debug_1_print_frequent_words(&param);
  }
  
  /*Step1.E Check frequent word numbers*/
  if (!param.freWordNum)
  {
    free_and_clean_step_0(&param);
    free_and_clean_step_1(&param);
    return 0;
  }
  
  /* ######## #### ## Step2 Cluster Candidates ## #### ######## */
  
  /*Step2.A Create cluster candidate sketch*/
  /*Tag: Optional, One pass over the data set*/
  if (param.clusterSketchSize)
  {
    step_2_create_cluster_candidate_sketch(&param);
  }
  
  /*Step2.B Finding cluster candidates*/
  /*Tag: One pass over the data set*/
  step_2_find_cluster_candidates(&param);
  
  /*Step2.C Aggregate support*/
  /*Tag: Optional*/
  if (param.bAggrsupFlag)
  {
    step_2_aggregate_supports(&param);
    str_format_int_grouped(digit, param.trieNodeNum);
    sprintf(logStr, "%s nodes in the prefix tree.", digit);
    log_msg(logStr, LOG_NOTICE, &param);
  }
  
  /*Step2.D Debug_1 mode: print cluster candidates*/
  /*Tag: Optional*/
  if (param.debug == 1)
  {
    debug_1_print_cluster_candidates(&param);
  }
  
  /* ######## #### ## Step3 Clusters ## #### ######## */
  
  /*Step3.A Find clusters*/
  log_msg("Finding clusters...", LOG_NOTICE, &param);
  
  param.clusterNum = step_3_find_clusters_from_candidates(&param);
  
  str_format_int_grouped(digit, param.clusterNum);
  sprintf(logStr, "%s cluster were found.", digit);
  log_msg(logStr, LOG_NOTICE, &param);
  
  /*Step3.B Join clusters*/
  /*Tag: Optional*/
  if (param.wordWeightThreshold)
  {
    step_3_join_clusters(&param);
  }
  
  /*Step3.C Print clusters*/
  if (param.clusterNum)
  {
    step_3_print_clusters(&param);
  }
  
  /* ######## #### ## Step4 Outliers ## #### ######## */
  
  /*Step4.A Find outliers*/
  /*Tag: Optional, One pass over the data set*/
  if (param.pOutlier)
  {
    log_msg("Finding outliers...", LOG_NOTICE, &param);
    
    outlierNum = step_4_find_outliers(&param);
    
    str_format_int_grouped(digit, outlierNum);
    sprintf(logStr, "%s outliers were outputted into file %s.", digit,
        param.pOutlier);
    log_msg(logStr, LOG_NOTICE, &param);
  }
  
  /* ######## #### ## Step5 Ending ## #### ######## */
  
  /*Step5.A Free and clean*/
  free_and_clean_step_0(&param);
  free_and_clean_step_1(&param);
  free_and_clean_step_2(&param);
  free_and_clean_step_3(&param);
  //no resource is allocated in step4.
  
  return 0;
}

