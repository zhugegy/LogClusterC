# 基于文字密度的文本数据挖掘工具

运行环境：Linux

如何编译：下载源文件后，命令行输入 gcc -O2 -o logclusterc *.c

此工具基于Risto Vaarandi和Mauno Pihelgas发明的LogCluster算法，这是一个基于文本密度的数据挖掘算法，主要应用于大规模日志的模式分析。

LogCluster算法的Perl版本: https://github.com/ristov/logcluster/

LogCluster算法的前身SLCT算法：http://ristov.github.io/slct/