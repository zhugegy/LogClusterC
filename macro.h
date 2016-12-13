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
 * File:   macro.h
 * 
 * Content: All the macros.
 *
 * Created on November 29, 2016, 7:30 PM
 */

#ifndef MACRO_H
#define MACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>    /* for syslog() */
  
/* ==== Configurable environment variables ==== */

/* Maximum length of a line. */
#define MAXLINELEN 10240

/* Maximum length of a word, should be at least MAXLINELEN+4. */
#define MAXWORDLEN 10248

/* Maximum number of words in one line. */
#define MAXWORDS 512

/* Maximum log message length. */
#define MAXLOGMSGLEN 256

/* Maximum number of () expressions in regexp. */
#define MAXPARANEXPR 100

/* Character that starts back-reference variables. */
#define BACKREFCHAR '$'

/* Maximum digit length, that is displayed in output. E.g. the number of
 frequent words and clusters.*/
#define MAXDIGITBIT 32

/* Separator character used for building hash keys of the cluster hash table. */
#define CLUSTERSEP '\n'

/* Maximum hash key length in cluster hash table. */
#define MAXKEYLEN 20480

/* Token length used in Join_Clusters. Token is an identifier for the words that
 is below word weight threshold. */
#define TOKENLEN 10

/* Word hash table's default size is 100000. */
#define DEF_WORD_TABLE_SIZE 100000

/* InitSeed is default to 1. It is used to generate random numbers, which help
 in the string hashing processes. */
#define DEF_INIT_SEED 1

/* Debug_2_interval defines after how many lines program status will refresh.
 Debug_3_interval is the time interval(seconds) to refresh status. */
#define DEBUG_2_INTERVAL 200000
#define DEBUG_3_INTERVAL 5

/* If --syslog option is given, log messages under or equal to
 DEF_SYSLOG_THRESHOLD will be written to Syslog. Setting it to LOG_NOTICE(5),
 (see syslog.h) can prevent potential massive LOG_INFO and LOG_DEBUG messages
 from polluting Syslog. */
#define DEF_SYSLOG_THRESHOLD LOG_NOTICE

/* If user doesn't append an argument after --syslog option, the default syslog
 facility is "local2". */
#define DEF_SYSLOG_FACILITY "local2"

/* Words are separated by space. Tab is not considered as a separator. */
//#define DEF_WORD_DELM "[ \t]+"
#define DEF_WORD_DELM "[ ]+"
  
/* ==== Type definitions ==== */

typedef unsigned long support_t;
typedef unsigned long tableindex_t;
typedef unsigned long linenumber_t;
typedef unsigned long wordnumber_t;

/* ==== Constant strings ==== */

#define VERSIONINFO "LogClusterC version 0.04, \
Copyright (C) 2016 Zhuge Chen, Risto Vaarandi and Mauno Pihelgas"

#define USAGEINFO "\n\
Options:\n\
--input=<file_name> or <file_pattern> ...\n\
--support=<support>\n\
--rsupport=<relative_support>\n\
--separator=<word_separator_regexp>\n\
--lfilter=<line_filter_regexp>\n\
--template=<line_conversion_template>\n\
--syslog=<syslog_facility>\n\
--wsize=<wordsketch_size>\n\
--wweight=<word_weight_threshold>\n\
--weightf=<word_weight_function> (1, 2)\n\
--wfilter=<word_filter_regexp>\n\
--wsearch=<word_search_regexp>\n\
--wreplace=<word_replace_string>\n\
--outliers=<outlier_file>\n\
--aggrsup\n\
--debug=<debug_level> (1, 2, 3)\n\
--byteoffset=<byte_offset>\n\
--csize=<clustersketch_size>\n\
--initseed=<seed>\n\
--wtablesize=<wordtable_size>\n\
--outputmode=<output_mode> (1)\n\
--detailtoken\n\
--help, -h\n\
--version\n\
\n\
"

#define HELPINFO "\n\
--input=<file_name> or <file_pattern>\n\
Find clusters from file, or files matching the <file_pattern>.\n\
For example, --input=/var/log/remote/*.log finds clusters from all files\n\
with the .log extension in /var/log/remote.\n\
This option can be specified multiple times.\n\
\n\
--support=<support>\n\
Find clusters (line patterns) that match at least <support> lines in input\n\
file(s). Each line pattern consists of word constants and variable parts,\n\
where individual words occur at least <support> times in input files(s).\n\
For example, --support=1000 finds clusters (line patterns) which consist\n\
of words that occur at least in 1000 log file lines, with each cluster\n\
matching at least 1000 log file lines.\n\
\n\
--rsupport=<relative_support>\n\
This option takes a real number from the range 0..100 for its value, and\n\
sets relative support threshold in percentage of total number of input lines.\n\
For example, if 20000 lines are read from input file(s), --rsupport=0.1 is\n\
equivalent to --support=20.\n\
\n\
--separator=<word_separator_regexp>\n\
Regular expression which matches separating characters between words.\n\
Default value for <word_separator_regexp> is \\s+ (i.e., regular expression\n\
that matches one or more whitespace characters).\n\
\n\
--lfilter=<line_filter_regexp>\n\
When clustering log file lines from file(s) given with --input option(s),\n\
process only lines which match the regular expression. For example,\n\
--lfilter='sshd\\[\\d+\\]:' finds clusters for log file lines that\n\
contain the string sshd[<pid>]: (i.e., sshd syslog messages).\n\
\n\
--template=<line_conversion_template>\n\
After the regular expression given with --lfilter option has matched a line,\n\
convert the line by substituting match variables in <line_conversion_template>.\n\
For example, if --lfilter='(sshd\\[\\d+\\]:.*)' option is given, only sshd\n\
syslog messages are considered during clustering, e.g.:\n\
Apr 15 12:00:00 myhost sshd[123]: this is a test\n\
When the above line matches the regular expression (sshd\\[\\d+\\]:.*),\n\
$1 match variable is set to:\n\
sshd[123]: this is a test\n\
If --template='$1' option is given, the original input line\n\
Apr 15 12:00:00 myhost sshd[123]: this is a test\n\
is converted to\n\
sshd[123]: this is a test\n\
(i.e., the timestamp and hostname of the sshd syslog message are ignored).\n\
Please note that <line_conversion_template> supports not only numeric\n\
match variables (such as $2 or ${12}), but also named match variables with\n\
$+{name} syntax (such as $+{ip} or $+{hostname}).\n\
This option can not be used without --lfilter option.\n\
\n\
--syslog=<syslog_facility>\n\
Log messages about the progress of clustering to syslog, using the given\n\
facility. For example, --syslog=local2 logs to syslog with local2 facility.\n\
You can also use this option with out argument, like '--syslog', which will\n\
set facility to local2.\n\
\n\
--wsize=<wordsketch_size>\n\
Instead of finding frequent words by keeping each word with an occurrence\n\
counter in memory, use a sketch of <wordsketch_size> counters for filtering\n\
out infrequent words from the word frequency estimation process. This\n\
option requires an additional pass over input files, but can save large\n\
amount of memory, since most words in log files are usually infrequent.\n\
For example, --wsize=250000 uses a sketch of 250,000 counters for filtering.\n\
\n\
--wweight=<word_weight_threshold>\n\
This option enables word weight based heuristic for joining clusters.\n\
The option takes a positive real number not greater than 1 for its value.\n\
With this option, an additional pass over input files is made, in order\n\
to find dependencies between frequent words.\n\
For example, if 5% of log file lines that contain the word 'Interface'\n\
also contain the word 'eth0', and 15% of the log file lines with the word\n\
'unstable' also contain the word 'eth0', dependencies dep(Interface, eth0)\n\
and dep(unstable, eth0) are memorized with values 0.05 and 0.15, respectively.\n\
Also, dependency dep(eth0, eth0) is memorized with the value 1.0.\n\
Dependency information is used for calculating the weight of words in line\n\
patterns of all detected clusters. The function for calculating the weight\n\
can be set with --weightf option.\n\
For instance, if --weightf=1 and the line pattern of a cluster is\n\
'Interface eth0 unstable', then given the example dependencies above,\n\
the weight of the word 'eth0' is calculated in the following way:\n\
(dep(Interface, eth0) + dep(eth0, eth0)\n\
+ dep(unstable, eth0)) / number of words = (0.05 + 1.0 + 0.15) / 3 = 0.4\n\
If the weights of 'Interface' and 'unstable' are 1, and the word weight\n\
threshold is set to 0.5 with --wweight option, the weight of 'eth0'\n\
remains below threshold. If another cluster is identified where all words\n\
appear in the same order, and all words with sufficient weight are identical,\n\
two clusters are joined. For example, if clusters 'Interface eth0 unstable'\n\
and 'Interface eth1 unstable' are detected where the weights of 'Interface'\n\
and 'unstable' are sufficient in both clusters, but the weights of 'eth0'\n\
and 'eth1' are smaller than the word weight threshold, the clusters are\n\
joined into a new cluster 'Interface (eth0|eth1) unstable'.\n\
\n\
--weightf=<word_weight_function>\n\
This option takes an integer for its value which denotes a word weight\n\
function, with the default value being 1. The function is used for finding\n\
weights of words in cluster line patterns if --wweight option has been given.\n\
If W1,...,Wk are words of the cluster line pattern, value 1 denotes the\n\
function that finds the weight of the word Wi in the following way:\n\
(dep(W1, Wi) + ... + dep(Wk, Wi)) / k\n\
Value 2 denotes the function that will first find unique words U1,...Up from\n\
W1,...Wk (p <= k, and if Ui = Uj then i = j). The weight of the word Ui is\n\
then calculated as follows:\n\
if p>1 then (dep(U1, Ui) + ... + dep(Up, Ui) - dep(Ui, Ui)) / (p - 1)\n\
if p=1 then 1\n\
\n\
--wfilter=<word_filter_regexp>\n\
--wsearch=<word_search_regexp>\n\
--wreplace=<word_replace_string>\n\
These options are used for generating additional words during the clustering\n\
process, in order to detect frequent words that match the same template.\n\
If the regular expression <word_filter_regexp> matches the word, all\n\
substrings in the word that match the regular expression <word_search_regexp>\n\
are replaced with the string <word_replace_string>. The result of search-\n\
and-replace operation is treated like a regular word, and can be used as\n\
a part of a cluster candidate. However, when both the original word and\n\
the result of search-and-replace are frequent, original word is given\n\
a preference during the clustering process.\n\
For example, if the following options are provided\n\
--wfilter='[.:]' --wsearch='[0-9]+' --wreplace=N\n\
the words 10.1.1.1 and 10.1.1.2:80 are converted into N.N.N.N and N.N.N.N:N\n\
Note that --wfilter option requires the presence of --wsearch and --wreplace,\n\
while --wsearch and --wreplace are ignored without --wfilter.\n\
\n\
--outliers=<outlier_file>\n\
If this option is given, an additional pass over input files is made, in order\n\
to find outliers. All outlier lines are written to the given file.\n\
\n\
--aggrsup\n\
If this option is given, for each cluster candidate other candidates are\n\
identified which represent more specific line patterns. After detecting such\n\
candidates, their supports are added to the given candidate. For example,\n\
if the given candidate is 'Interface * down' with the support 20, and\n\
candidates 'Interface eth0 down' (support 10) and 'Interface eth1 down'\n\
(support 5) are detected as more specific, the support of 'Interface * down'\n\
will be set to 35 (20+10+5).\n\
\n\
--debug=<debug_level> (1,2,3)\n\
Increase logging verbosity by generating debug output. Debug level 1 displays\n\
a summary after each phase is done. Debug level 2 displays the processing\n\
status after every 200,000 lines are analysed. Debug level 3 displays the\n\
processing status every 5 seconds. When analysing large log files bigger than\n\
1GB, debug level 2 or 3 is sugguested.\n\
For the sake of consistency with Perl version, you can also use this option\n\
without argument, like '--debug', which will set debug level to 1.\n\
\n\
--byteoffset=<byte_offset>\n\
When processing the input file(s), ignore the first <byte offset> bytes of \n\
every line. This option can be used to filter out the possibly irrelevant\n\
information in the beginning of every line (e.g., timestamp and hostname). The\n\
default value for the option is zero, i.e., no bytes are ignored.\n\
\n\
--csize=<clustersketch_size>\n\
The size of the cluster candidate summary vector(sketch). The default value for\n\
the option is zero, i.e., no summary vector will be generated. This option and\n\
the option --aggrsup are mutually exclusive, since -aggrsup requires the\n\
presence of all candidates in order to produce correct results, but when the\n\
summar vector is employed, not all candidates are inserted into the candidate\n\
table.\n\
\n\
--initseed=<seed>\n\
The value that is used to initialize the rand(3) based random number generator\n\
which is used to generate seed values for string hashing functions inside\n\
LogCluster. The default value for the option is 1.\n\
\n\
--wtablesize=<wordtable_size>\n\
The number of slots in the vocabulary hash table. The default value for the\n\
option is 100,000.\n\
\n\
--outputmode=<output_mode> (1)\n\
This program outputs the clusters with a support value descending order. This\n\
option changes the way of outputing clusters. When output mode is set to 1,\n\
the clusters will be sorted by their constant number, from small to big. In\n\
another word, the clusters will be sorted by their complexity, from simple to\n\
complex.\n\
You can also use this option with out argument, like '--outputmode', which will\n\
set output mode to 1.\n\
\n\
--detailtoken\n\
If Join_Cluster heuristic('--wweight' option) is used, this option can make the\n\
output more detailed. For the sake of simplicity, by default, if a token has\n\
only one word, it will not be surrounded by parentheses. With this option on,\n\
as long as it is a token, there will be parentheses surrounded, indicating\n\
it is under word weight threshold.\n\
For example, if \"interface\", \"up\" and \"down\" are under word weight\n\
threshold. By default, output is\n\
Interface eth0 (up|down)\n\
With this option, output is\n\
(Interface) eth0 (up|down)\n\
This option is meaningless without '--wweight' option.\n\
\n\
--help, or -h\n\
Print this help.\n\
\n\
--version\n\
Print the version information.\n\
"

static char *pSyslogFacilityList[] =
{
  "kern",
  "user",
  "mail",
  "daemon",
  "auth",
  "syslog",
  "lpr",
  "news",
  "uucp",
  "cron",
  "authpriv",
  "ftp",
  "ntp",
  "log_audit",
  "log_alert",
  "cron",
  "local0",
  "local1",
  "local2",
  "local3",
  "local4",
  "local5",
  "local6",
  "local7"
};

/* ==== Error information ==== */

#define MALLOC_ERR_6000 "malloc() failed! Function: main()."
#define MALLOC_ERR_6001 "malloc() failed! Function: init_input_parameters()."
#define MALLOC_ERR_6002 "malloc() failed! Function: create_trie_node()."
#define MALLOC_ERR_6003 "malloc() failed! Function: build_prefix_trie()."
#define MALLOC_ERR_6004 "malloc() failed! Function: build_input_file_chain()."
#define MALLOC_ERR_6005 "malloc() failed! Function: build_template_chain()."
#define MALLOC_ERR_6006 "malloc() failed! Function: parse_options()."
#define MALLOC_ERR_6007 "malloc() failed! Function: add_elem()."
#define MALLOC_ERR_6008 "malloc() failed! Function: find_words()."
#define MALLOC_ERR_6009 "malloc() failed! Function: create_cluster_instance()."
#define MALLOC_ERR_6010 "malloc() failed! Function: create_cluster_with_token_instance()."
#define MALLOC_ERR_6011 "malloc() failed. Function: adjust_cluster_with_token_instance()."
#define MALLOC_ERR_6012 "malloc() failed. Function: debug_1_print_frequent_words()."
#define MALLOC_ERR_6013 "malloc() failed. Function: debug_1_print_cluster_candidates()."
#define MALLOC_ERR_6014 "malloc() failed. Function: step_1_create_word_sketch()."
#define MALLOC_ERR_6015 "malloc() failed. Function: step_1_create_vocabulary()."
#define MALLOC_ERR_6016 "malloc() failed. Function: step_2_create_cluster_sketch()."
#define MALLOC_ERR_6017 "malloc() failed. Function: step_2_find_cluster_candidates()."
#define MALLOC_ERR_6018 "malloc() failed. Function: print_clusters_default_1()."
#define MALLOC_ERR_6019 "malloc() failed. Function: print_clusters_if_join_cluster_default_0()."
#define MALLOC_ERR_6020 "malloc() failed. Function: __print_clusters_if_join_cluster_default_0()."

/* ==== Macro function ==== */

#define ARR_SIZE(a) (sizeof((a))/sizeof((a[0])))


#ifdef __cplusplus
}
#endif

#endif /* MACRO_H */

