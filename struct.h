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
 * File:   struct.h
 * 
 * Content: All the structs.
 *
 * Created on November 29, 2016, 7:32 PM
 */

#ifndef STRUCT_H
#define STRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "macro.h"
#include <regex.h>
#include <time.h>

/* ==== Struct definitions ==== */

struct Cluster;    //declaration

/* This struct stores input file(s)'s path(s).
 
 lineNumber is the count of lines of this file. It is used for debug purpose,
 helps in the calculation of the mining process status. */
struct InputFile {
  char *pName;
  linenumber_t lineNumber;
  struct InputFile *pNext;
};

/* This struct stores elements that are placed into hash tables. One element can
 be a word or a cluster candidate.
 
 pKey is the identifier(description).
 
 count increments every time when element's identifier occurs.
 
 number is a sequential and unique ID, which is assigned to an element when it
 first appears.
 
 If an element is a cluster candidate, there will be a dedicated struct Cluster
 assigned to it, which contains more detailed information about this cluster
 candidate. Between Elem and Cluster, there is a bidirectional link pointing to
 each other.
 
 pNext points to the next element that shares the same hash slot, if there is
 any. */
struct Elem {
  char *pKey;
  support_t count;
  wordnumber_t number;
  struct Cluster *pCluster;
  struct Elem *pNext;
};

/* This struct stores information of templates, which is set with option
 '--template'. */
struct TemplElem {
  char *pStr;
  int data;
  struct TemplElem *pNext;
};

/* Word frequency statistics. */
struct WordFreqStat {
  wordnumber_t ones;
  wordnumber_t twos;
  wordnumber_t fives;
  wordnumber_t tens;
  wordnumber_t twenties;
};

/* This struct stores detailed information about cluster candidates(potential
 clusters). It has a bidirectional link with {struct Elem}.
 
 constants is the number of frequent words in this cluster candidate.
 
 count increments every time when this cluster candidate occurs.
 
 fullWildcard stores the wildcard information of this cluster candidate,
 fullWildcard[0] is the number of minimum wildcard in tail. fullWildcard[1] is
 the number of maximum wildcard in tail. fullWildcard[2] is the number of
 minimum wildcard of the 1st constant. fullWildcard[3] is the number of the
 maximum wildcard of the 1st constant. fullWildcard[4] and fullWildcard[5]
 are for the 2nd constant, and so on..
 
 For example, if a cluster candidate is "*{8,9} Interface *{0,7} break *{2,3}",
 its fullWildcard will store 2,3,8,9,0,7.
 
 pElem is the bidirectional link, towards the element which is stored in cluster
 hash table.
 
 ppWord is an array that stores each constant's element, which is stored in word
 hash table.
 
 If Aggregate_Supports heuristics is used('--aggrsup' option), pLastNode is the
 address of the cluster candidate's last node in prefix tree. According to this
 address, this cluster candidate's parent and other relatives can be back
 tracked. Prefix tree(aka trie) is build for efficiently looking up for cluster
 candidates that have a common prefix, thus efficiently checking if one cluster
 candidate's support value can be aggregated to another.
 
 If Join_Clusters heuristics is used('--wweight' option), bIsJoined is the flag
 indicates whether this cluster has token (word that is under word weight
 threshold). If a cluster has token, it will be ignored in the process of
 printing clusters. Instead, program will print the joined cluster which is
 stored in another struct({struct ClusterWithToken}) which is dedicated to
 Join_Clusters heuristics.
 
 pNext: Besides the cluster hash table, which stores {struct Elem} address, we
 have an organized array table(pClusterFamily[]) to store {struct Cluster}
 address, assigning each cluster candidate into slot according to it constants(
 the number of frequent words). pNext stores the address of next
 {struct Cluster} sharing the same slot in pClusterFamily[].
 
 For example, cluster candidates with description "Interface *{1,2} down" and
 "User login *{1,1}" share the same slot pClusterFamily[2]. */
struct Cluster {
  int constants;
  support_t count;
  int *fullWildcard;
  struct Elem *pElem;
  struct Elem **ppWord;
  struct TrieNode *pLastNode;
  char bIsJoined;
  struct Cluster *pNext;
};

/*This struct is dedicated to Join_Clusters heuristics.
 
 More details are in the description of {struct ClusterWithToken}. */
struct Token {
  struct Elem *pWord;
  struct Token *pNext;
};

/*This struct is dedicated to Join_Clusters heuristics.
 
 If a cluster has token, this cluster's bIsJoined will be marked, and this
 cluster's information stored in {struct Cluster} will be copied into a new
 struct {struct ClusterWithToken}.
 
 Compared to {struct Cluster}, there are only two different attributes: pNext
 and ppToken. The other attributes store the same information as in former
 {struct Cluster}.
 
 pNext: We continue to use the same way of storage organization as
 {struct Cluster}, but only uses a different name pClusterWithTokenFamily[].
 pNext stores the address of next {struct ClusterWithToken} that shares the same
 slot in pClusterWithTokenFamily[].
 
 ppToken: For every cluster that has token and thus transfered into
 {struct ClusterWithToken}, we malloc an array according to its constants. Every
 constant has a slot to store tokens, which contain the original words(which are
 frequent words, but are under word weight threshold). When printing clusters in
 pClusterWithTokenFamily[], we can know the original words from ppToken, and
 print strings contain word summary, such like:
 
 Interface *{2,3}(A|B|C) *{0,2}  
   
 A, B, and C represent the words that are under word weight threshold, and thus
 joined.
 */
struct ClusterWithToken {
  int constants;
  support_t count;
  int *fullWildcard;
  struct Elem *pElem;
  struct Elem **ppWord;
  struct TrieNode  *pLastNode;
  char bIsJoined;
  struct ClusterWithToken *pNext;
  
  /*The order of elements in this struct matters. In later process, type
   {struct ClusterWithToken} and type {struct Cluster} will be force
   transfered to each other.*/
  struct Token **ppToken;
};

/* This struct is dedicated to Aggregate_Supports heuristics.
 
 Every node is a constant or wildcard(*{min,max}) in cluster candidates.
 
 Every node has only one pParent, pNext and pChlid.
 
 pIsEnd indicates a cluster candidates ends in this node, and stores the address
 of {struct Cluster}. Otherwise, it shall be null(0).
 
 When node is a constant(frequent word), pWord stores the address of
 {struct Elem}.
 
 When node is a wildcard, we store its minimum and maximum value in wildcardMin
 and wildcardMax.
 
 hashValue is for efficiently inserting and looking up. Only if their 
 hashValue-s are equal, we will then use strcmp() or wildcardMin/Max compare to
 see if the node to be inserted is already exist, because strcmp() is an 
 expensive process regarding computing speed.
 
 When node is a constant, hashValue is calculated by str2hash() function, with a
 hash module size (frequent word number) * 3.
 
 When node is a wildcard, its hashValue is (frequent word number) * 3. Thus, all
 wildcards, regardless of their minimun and maximum, have the same hashValue.
 
 Nodes in the same horizontal level and with a common parent, are arranged
 from left to right with a descending hashValue. Therefore, when inserting new
 node in prefix tree, we check if it already exist by comparing hashValue, with
 an order from big to small. In other words, wildcards are always in the front
 part of comparison, which takes advantage of the statistics feature of cluster 
 candidates.
 */
struct TrieNode {
  struct TrieNode *pParent;
  struct TrieNode *pNext;
  struct TrieNode *pChild;
  struct Cluster *pIsEnd;
  struct Elem *pWord;
  int wildcardMin;
  int wildcardMax;
  wordnumber_t hashValue;
};

/* This struct stores parameters. It can be considered as a storage for global
 variables. Sorry that so many parameters were put into this struct. For the 
 sake of manageability of future updates, this issue would be properly fixed in 
 following updates.*/
struct Parameters {
  /* >>> Below are parameters that can be changed by command line options. */
  char bAggrsupFlag;
  char bDetailedTokenFlag;
  char *pDelim;
  char *pFilter;
  char *pOutlier;
  char *pSyslogFacility;
  char *pWordFilter;
  char *pWordReplace;
  char *pWordSearch;
  double pctSupport;
  double wordWeightThreshold;
  int byteOffset;
  int debug;
  int outputMode;
  int wordWeightFunction;
  struct InputFile *pInputFiles;
  struct TemplElem *pTemplate;
  support_t support;
  tableindex_t clusterSketchSize;
  tableindex_t wordSketchSize;
  tableindex_t wordTableSize;
  unsigned int initSeed;
  
  /* >>> Below are parameters that are not visible to user. */
  
  /* >>>>>> Common usage */
  
  char bSyslogFlag;
  
  /* In order to avoid unnecessary iterations to pClusterFamily[], who has a 
   size of MAXWORDS + 1, which equals to 513, biggestConstants stores the 
   biggest constants ever happened to cluster candidates. A normal log line with
   a normal length, usually has constants no more than 30. */
  int biggestConstants;
  
  /* syslogFacilityNum is calculated according to user input and file syslog.h. 
   */
  int syslogFacilityNum;
  
  /* syslogThreshold is default to LOG_NOTICE(5). */
  int syslogThreshold;
  
  regex_t delim_regex;
  regex_t filter_regex;
  
  /* pClusterFamily[] stores {struct Cluster} according to their constants. */
  struct Cluster *pClusterFamily[MAXWORDS + 1];
  
  /* ppClusterTable stores the pointer of every cluster candidate elem. So
   does ppWordTable. */
  struct Elem **ppClusterTable;
  struct Elem **ppWordTable;
  
  support_t *pClusterSketch;
  support_t *pWordSketch;
  tableindex_t clusterSketchSeed;
  tableindex_t clusterTableSeed;
  tableindex_t clusterTableSize;
  tableindex_t wordSketchSeed;
  tableindex_t wordTableSeed;
  
  /* These four numbers stores results that are produced in the program's 
   process. */
  wordnumber_t clusterCandiNum;
  wordnumber_t clusterNum;
  wordnumber_t freWordNum;
  wordnumber_t trieNodeNum;
  
  /* >>>>>> Used in Aggregate_Supports heuristics. */
  
  /* prefixWildcardMax/Min are used for temporary storage, when comparing
   wildcard nodes to see if it already exist. */
  int prefixWildcardMax;
  int prefixWildcardMin;
  
  /* pPrefixRet is used for temporary storage. */
  struct TrieNode *pPrefixRet;
  
  /* pPrefixRoot is the root of the prefix tree. */
  struct TrieNode *pPrefixRoot;
  
  tableindex_t prefixSketchSeed;
  
  /* prefixSketchSize will be set to (frequent word number) * 3. */
  wordnumber_t prefixSketchSize;
  
  wordnumber_t wildcardHash;
  
  /* >>>>>> Used in Join_Clusters heuristics. */
  
  /* The content of token. Default is "token". If "token" is already among
   the frequent words, random string that is not among frequent words will be
   generated and replace it.*/
  char token[TOKENLEN];
  
  /* Temporarily mark a cluster's constant as token, for upcoming process's
   usage. If this cluster has token, tokenMarker[0] is set to 1.
   The corresponding constants's tokenMarker[] slot will also be set to 1. */
  char tokenMarker[MAXWORDS + 1];
  
  /* When we calculate a cluter's constants' word weight, using function_2,
   we will get every unique word out of constants. In order to avoid doing
   this job every time for each constant in the same cluster(the result will
   be the same), we use this pointer to indicate current cluster that is
   under processing. We do this getting_unique_words job again only if our
   target cluster differs from pCurrentCluster. */
  struct Cluster *pCurrentCluster;
  
  /* An array storages Clusters that have token. It's similar as
   pClusterFamily[]. */
  struct ClusterWithToken *pClusterWithTokenFamily[MAXWORDS + 1];
  
  /* JoinedClusterInput/OutputNum are used for statistics purpose. They
   record how many clusters have been joined, and how many new clusters the
   joined clusters have generated. */
  tableindex_t joinedClusterInputNum;
  tableindex_t joinedClusterOutputNum;
  
  /* Word Dependency Matrix Breadth will be (number of frequent words) + 1. */
  tableindex_t wordDepMatrixBreadth;
  
  /* Short for wordNumberStorage, used for temporarily storing the constants'
   numbers, as their identifier. The numbers will be used to update word
   dependency matrix. */
  wordnumber_t wordNumStr[MAXWORDS + 1];
  
  /* This matrix is a square matrix. We need one pass over the data set to get
   this matrix. The matrix will be updated each time after each reading of a
   single log line. To optimize performance, this pass over the data set is
   integrated with find_cluster_candidates() (doing two different jobs at
   the same pass over the data set). */
  wordnumber_t *wordDepMatrix;
  
  /* >>>>>> Used in '--debug' option. */
  
  /* Temporarily storage cluster candidates' description before printing them
   out in debug mode. */
  char clusterDescription[MAXLOGMSGLEN];
  
  /* linecount is the total number of lines in all input files. It is used
   for calculation of processing status. */
  support_t linecount;
  
  /* The total time of passing over the whole data set. Program will calculate 
   this number before starting the data mining task. */
  int dataPassTimes;
  
  /* Total line number of all input files. */
  support_t totalLineNum;
  char totalLineNumDigit[MAXDIGITBIT];  //digit format with comma, e.g. 123,456
  
  time_t timeStorage;
  
  /* >>>>>> Used in '--wfilter/--wsearch/--wreplace'options. */
  
  regex_t wfilter_regex;
  regex_t wsearch_regex;
  
  //char *pWordFilter;
  //char *pWordSearch;
  //char *pWordReplace;
  
  /* To avoid modifying original words that is gotten from log lines, we use
   tmpStr to make a copy of it and do modification on this copy.
   With current functions, it is fine to direct modify original words, but
   we define and use this tmpStr, in case there are other functions being
   added in the future that are sensitive to this issue. */
  char tmpStr[MAXWORDLEN];
  
};


#ifdef __cplusplus
}
#endif

#endif /* STRUCT_H */

