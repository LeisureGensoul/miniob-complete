# Finish the date
1. 要求实现日期类型字段。
2. 当前已经支持了int、char、float类型，在此基础上实现date类型的字段。
3. date测试可能超过2038年2月，也可能小于1970年1月1号。注意处理非法的date输入，需要返回FAILURE。
4. 这道题目需要从词法解析开始，一直调整代码到执行阶段，还需要考虑DATE类型数据的存储。
5. 需要考虑date字段作为索引时的处理，以及如何比较大小;
6. 这里没有限制日期的范围，需要处理溢出的情况。 - 需要考虑闰年。

宏观思路:该数据库本身已经支持int类型,从头实现date类型难度较大.考虑到date本身和int之间便有足够的联系,并且相关运算(如比较大小等)也比较适配,所以可以在语法解析部分对已经识别出来的日期字符串开始进行相应year,month,day的识别(借用一个辅助函数find定位日期字符串中-的位置,随后分别对ymd进行赋值),识别出来以后,在仿照int相关的实现对date进行支持(期间注意合法性的判断即可),将date存储为int.

src/observer/sql/parser/lex_sql.l
src/observer/sql/parser/yacc_sql.y
src/observer/sql/parser/parse.cpp

src/observer/sql/expr/tuple_cell.cpp
src/observer/sql/stmt/filter_stmt.cpp

(以下需要在相关位置添加dates属性值的索引)
src/observer/sql/parser/parse_defs.h
src/observer/storage/common/condition_filter.cpp
src/observer/storage/common/field_meta.cpp
src/observer/storage/default/default_storage_stage.cpp
src/observer/storage/index/bplus_tree.h

src/observer/sql/parser/parse_stage.cpp
首先需要处理相应的词法解析和语法解析.
    在src/observer/sql/parser/lex_sql.l中添加token-DATE_T用来指示日期类型,并且添加正则表达式来匹配语句中出现的日期字样,即DATE_STR.
    随后在src/observer/sql/parser/yacc_sql.y中添加相应的DATE_T的赋值以及对DATE_STR的处理.

随后在各个地方添加属性值类型dates,即parse_defs.h,condition_filter.cpp,field_meta.cpp,default_storage_stage.cpp,bplus_tree.h

然后,在~/sql/parser/parse.cpp中添加对date字段的初始化操作,即value_init_date,并且在这里完成合法性判断(如闰年),随后分配相应的内存.

最后还需要完成两个部分,一是需要显示时,如何将其转换为字符串;二是如何对date进行相应的判断.(均在~/sql/expr/tuple_cell.cpp中)在to_string函数中添加DATES的处理,即将data字段(根据存储时的设置)转换为日期格式串.同时在compare函数中添加DATES选项,经过分析,这里可以继续复用int的比较函数.

最后还需要在~/sql/stmt/filter_stmt.cpp中添加对两个类型的判断,检查两个类型是否能够比较.