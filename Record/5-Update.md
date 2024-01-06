# Finish the update

1. update单个字段即可。
2. 可以参考insert_record和delete_record的实现。
3. 目前仅能支持单字段update的语法解析，但是不能执行。
4. 需要考虑带条件查询的更新，和不带条件的更新。

宏观思路:仿照insert_record和delete_record的实现,在相应位置处添加适配的功能支持.

src/observer/sql/executor/execute_stage.cpp -> handle_request/do_update
src/observer/sql/operator/update_operator.cpp -> open/next/close
src/observer/sql/stmt/stmt.cpp -> create_stmt
src/observer/sql/stmt/update_stmt.cpp -> create
src/observer/storage/common/table.cpp -> update_record
src/observer/storage/trx/trx.cpp -> update_record

首先需要定位到src/observer/sql/stmt/stmt.cpp添加对update语句的功能支持,即添加SCF_UPDATE部分.随后在同目录下完善update_stmt部分(实现TODO,即create函数).

然后便是在src/observer/sql/executor/execute_stage.cpp中的handle_request处添加UPDATE语句的处理,调用do_update语句实现相应功能.

在该函数中,我们需要实际的操作类(即待实现的UpdateOperator)进行实际的数据库操作,于是需要在~/sql/operator/update_operator.cpp(新建项)中完成class UpdateOperator : public Operator该类的具体实现(主要关注open,next,close三个主要函数).

随后,我们还需要对具体的表进行操作(即UpdateOperator中指向table中的一些方法,以期完成具体的数据操作.),因此我们需要在src/observer/storage/common/table.cpp中实现update_record函数.

除此之外,我们还需要在src/observer/storage/trx/trx.cpp中添加相对应的事务支持.(仿照insert和delete的实现)