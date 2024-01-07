# Finish the unique

1. 实现唯一索引：create unique index。
2. 需要支持多列的唯一索引。
3. 需要支持查看索引。
4. 不考虑在已有重复数据的列上建立唯一索引的情况。

src/observer/sql/executor/execute_stage.cpp
src/observer/sql/parser/lex_sql.l
src/observer/sql/parser/yacc_sql.y
src/observer/storage/common/index_meta.cpp
src/observer/storage/common/table_meta.cpp
src/observer/storage/common/table.cpp
src/observer/storage/default_handler.cpp

src/observer/storage/common/index_meta.cpp
src/observer/storage/common/table.cpp
src/observer/storage/index/bplus_tree_index.cpp
src/observer/storage/index/bplus_tree.cpp



(完善了一下show index的逻辑,并且新增了对unique的词法和语法支持,同时在一些必要的数据结构中添加上unique字段进行标识,并新增一些辅助函数,更改一些函数的参数.)

(在之前的实现中,我们虽然能够建立相关的表和索引,并且能够对unique属性进行标识,但是在根据样例进行输入时,我们发现它无法处理insert时所出现的错误,即还是会把相关的unique属性插入进去,通过追踪RECORD_DUPLICATE_KEY的位置,我们可以发现原本的代码对于处理重复值的操作)
-> src/observer/storage/index/bplus_tree.cpp -> insert_entry_into_leaf_node() -> LeafIndexNodeHandler:leaf_node.lookup() -> class KeyComparator的比较逻辑,可知如果v1=v2,则result=0,因此会返回其他比较函数的值,故因此这里鲜有duplicate的错误,但是如果该值为unique,则需要直接返回相等,触发duplicate的异常,所以在这里添加上unique_的判断比较(若为零,则直接返回result的值,如果result此时不为零则寻得一个插入位置,否则触发duplicate异常). 
同时我们需要为index提供unique支持,所以BplusTreeIndex::create以及IndexMeta::init,KeyComparator::init中我们需要添加一个unique的参数.