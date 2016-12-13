# C-based Implementation of LogCluster Algorithm

**How to manually compile the source files:**
In terminal, change directory to this folder and execute "gcc -O2 -o logclusterc *.c" command. The executable file named "logclusterc" then will be generated.

LogCluster is a density-based data clustering algorithm for event logs, introduced by Risto Vaarandi and Mauno Pihelgas in 2015.
 
A detialed discussion of the LogCluster algorithm can be found in the paper (http://ristov.github.io/publications/cnsm15-logcluster-web.pdf) published at CNSM 2015.

The C-based implementation of LogCluster algorithm is called LogClusterC.

LogClusterC borrows lots of source code from another open source data mining tool SLCT: http://ristov.github.io/slct/ .

The information of LogCluster algorithm and its prototype implementation in Perl: http://ristov.github.io/logcluster/ .

The history versions of LogCluster in Perl: https://github.com/ristov/logcluster/releases .

All the functions in LogCluster Perl version 0.03 are supported in LogClusterC. The basic functions and heuristics functions (options: --input, --rsupport, 
--support, --aggrsup, --wweight) are tested via various experiments in thesis.

The corresponding thesis，the bash script to run the experiment and the raw plain text outputs in the experiment are stored in another GitHub repository: https://github.com/zhugehq/thesis-project-logclusterc-experiment-data .