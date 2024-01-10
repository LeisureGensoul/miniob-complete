# Finish the null.

(通过分析expression的样例可知,我们需要解决一些1/0的特殊情况,由此可以想到借用null来实现,于是在初步实现null的情况下,我们修缮了expression所出现的bug,并且在运行过程中还发现了一个极不仔细的错误,变量的初始化漏了一个关键的变量,即parser.cpp中minus变量.)(除此之外,还出现了segmentation fault,详见table_scan_operator.cpp的位置)