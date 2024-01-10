# Finish the expression.

1. 查询中支持运算表达式，这里的运算表达式包括 +-*/。运算只需要考虑整数和浮点数的转换。
2. 仅支持基本数据的运算即可，不对date字段做考察。 运算出现异常，按照NULL规则处理。
3. 只会出现在select语句中。
4. 需要考虑聚合运算。

src/observer/sql/executor/executor_stage.cpp -> do_select
src/observer/sql/expr/expression.h -> ExprType/Expression/FieldExpr/BinaryExpression

src/observer/sql/expr/tuple_cell.cpp -> sub/mul/div/add
src/observer/sql/operator/project_operator.cpp -> gen_project_name/add_projection

src/observer/sql/parser/lex_sql.l
src/observer/sql/parser/yacc_sql.y
src/observer/sql/parser/parse_defs.h -> ExpOp/ExpType/_UnaryExpr/_BinaryExpr/_ProjextCol/Selects
src/observer/sql/parser/parse.cpp -> attr/value/unary_expr/projectcol/binary_expr/condition/expr相关的功能函数.

src/observer/sql/stmt/filter_stmt.cpp -> create_expression/create_filter_unit
src/observer/sql/stmt/select_stmt.cpp -> wildcard_fields/gen_project_expression
src/observer/storage/common/condition_filter.cpp -> init

宏观思路:首先先实现相应的词法和语法支持(同时为了功能的实现,我们需要在parse.cpp中增加一些辅助函数).由于只会出现在select语句中,因此我们需要从追踪select语句开始,在原本的映射操作中,仅仅支持最基础的那一类,因此我们需要将其进行扩展(将原本只是支持单个查询列的功能扩展为支持相应的expression,即将query_fields用projects替代,表示一个Expression数组,同时跟踪其位置,我们还需要完善其子类FieldExpr和BinaryExpression,用以支持列属性出现的表达式的取值以及实现一个二元操作表达式的功能.其中BinaryExpression又借用TupleCell中自定义实现的函数add/sub/mul/div).于是相对应的我们还需要修改查询的相关函数,即add_projection中,将TupleCellSpec的参数修改为上面采用的projects,然后利用自定义的函数gen_project_name生成和表达式契合的列名.除此之外,我们还需要修改select语句(即select_stmt)的实现,使其支持Expression的功能.
其次便是修改~/stmt/filter_stmt.cpp中create_filter_unit部分,利用create_expression函数生成相适应的左右值的表达式,然后再进行赋值.同时,还需要修改~/storage/common/condition_filter.cpp中的DefaultConditionFilter::init函数,(支持where后的expression部分)