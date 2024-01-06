# Finish the drop-table

1. 删除表，清除表相关的资源。
2. 要删除所有与表关联的数据，不仅包括磁盘中的文件，还包括内存中的索引等数据。

src/observer/sql/executor/execute_stage.cpp -> handle_request/do_drop_table
src/observer/storage/common/db.cpp -> drop_table
src/observer/storage/common/table.cpp -> remove
src/observer/storage/trx/trx.cpp -> delete_table

宏观思路:追踪程序的运行逻辑,在相应的位置添加具体的功能支持.

通过跟踪相关运行流程可知,我们首先需要定位到src/observer/sql/executor/execute_stage.cpp,在这里实现了相对应的功能.

应该注意到其中SCF_DROP_TABLE为空实现,于是我们需要从这里添加对该功能的支持,仿照已经实现了的逻辑,我们在这个case中添加函数入口,调用do_drop_table函数,处理相应的session_event:sql_event.该函数位于~/sql/executor/execute_stage.cpp中,同时在.h文件中添加声明.
    -> 此函数额外指向位于~/storage/trx/trx.cpp中的delete_table函数(自实现),处理相应的事务.
    -> 同时调用db中的drop_table函数(~/storage/common/db.cpp中增加实现)进行实际的删除操作.

在~/storage/common/db.cpp中具体实现drop_table函数,并在相应的头文件db.h中添加相应的声明.
    -> 该函数额外指向同目录下table.cpp中所实现的remove函数,完成实际的数据库操作.
    -> 删除相应的index文件,关闭data文件,然后remove data文件,再remove meta文件.