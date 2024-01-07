# Finish the select-tables

1. 当前系统支持单表查询的功能，需要在此基础上支持多张表的笛卡尔积关联查询，并考虑基本类型转换。
2. 需要实现select * from t1,t2; select t1.*,t2.* from t1,t2;以及select t1.id,t2.id from t1,t2;查询可能会带条件。
3. 查询结果展示格式参考单表查询。每一列必须带有表信息

src/observer/sql/executor/execute_stage.cpp -> try_to_create_index_scan_operator/split_filters/do_join/do_select

src/observer/sql/expr/tuple.h -> get_record/set_record/set_right_record/CompoundTuple



src/observer/sql/operator/join_operator.cpp -> open/fetch_right_table/print_info/next/close/current_tuple

src/observer/sql/operator/predicate_operator.cpp -> do_predicate/next

src/observer/sql/operator/project_operator.cpp -> add_projection/

src/observer/sql/parser/lex_sql.l
src/observer/sql/parser/yacc_sql.y

src/observer/sql/stmt/select_stmt.cpp -> create

src/observer/storage/record/record.h -> CompoundRecord

宏观思路:首先要在词法和语法的分析上增加对相关attr_list的支持,使其可以识别出多张表,然后考虑到运行原本的运行逻辑,是在src/observer/sql/executor/execute_stage.cpp的do_select中实现对表的查询,这里可以扩展其功能,通过该文件下do_join函数(借由split_filters函数)将多张表进行笛卡尔积关联(故因此需要完善JoinOperator的相关内容),形成复合元组,然后再进行后续操作(此处借由实现的CompoundTuple指代所形成的复合元组).
(然后修改与之相关的函数参数以及类型说明即可).

首先考虑到多表查询会涉及到笛卡儿积(及相关的join操作),因此需要先完善join_operator的内容(实现相关的主要函数,如open/close/next等等).
然后考虑到多表查询应该归属于查询部分,因此可以考虑修改do_select部分,实现多表查询功能.同时还要注意修改add_projection部分,使得可以进行正确的project(对单表来说,展示的(alias)字段总是字段名称/对多表查询来说,展示的alias需要带表名字).
    -> 修改do_select部分,根据select_stmt中tables的size大小判断是否是多表查询,用is_single_table记录下这一信息.倘若是多表查询,则进行笛卡尔积关联,借用本文件下所使用的do_join函数(以及split_filters函数).
    -> 随后进行正确的project,在project_oper的add_projection函数中增加对is_single_table的参数引用.
    -> 修改add_projection的内容,依据is_single_table实现不同的字段名称赋值.

最后还需要注意,为了支持笛卡尔积关联后的复合元组,我们选择引入一个新的class CompoundTuple : public Tuple(JoinOperator中利用他实现与笛卡尔积相关部分的关联),同时增加get_record/set_record/set_right_record三个函数(参数均与CompoundRecord相关)(在record.h中添加CompoundRecord声明,即与record相关的vector),以其获得正确的功能.