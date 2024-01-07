# Finish the show index

1. 查看某表的索引信息。
2. 输出固定为表名,是否唯一索引,索引名称,列名和列在索引中的序号(针对多列索引)

src/observer/sql/executor/execute_stage.cpp -> handle_request/do_show_index

src/observer/sql/parser/parse_defs.h
src/observer/sql/parser/yacc_sql.y

src/observer/storage/common/index_meta.cpp
src/observer/storage/common/table_meta.cpp

宏观思路:首先需要实现show index的语法解析,故首先需要在yacc_sql.y中添加相应解析.然后仿照drop的写法,首先在handle_request中为SCF_SHOW_INDEX提供功能函数入口,指向do_show_index,该函数调用table_meta中自定义的show_index完成对应功能.

首先在yacc_sql.y中增加语法解析,然后定位到主要流程handle_request中,补全SCF_SHOW_INDEX的内容,并调用本文件下实现的do_show_index函数.
该函数首先判断table是否存在,然后调用table_meta中的功能函数show_index(自定义的),首先输出第一行的提示信息,然后循环输出该表中存在的index(利用index_meta中自定义的函数show,打印出相关信息).