# Update log
## 2016-04-12
1. Changed the process name to "logclusterc" in syslog utility.

2. Fixed the bug: When there is zero cluster found, segment 11 error happens. This is because sorting function tries to sort an empty cluster array.

## 2016-12-01
1. Optimized memory consumption by modifying the content of {struct Cluster} and {struct ClusterWithToken}.

2. Divided the one-file source code into different modules.
