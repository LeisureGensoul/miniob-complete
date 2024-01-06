#pragma once

#include "sql/operator/operator.h"
#include "rc.h"

class UpdateStmt;

class UpdateOperator : public Operator {
public:
  UpdateOperator(UpdateStmt *update_stmt, Trx *trx) : update_stmt_(update_stmt), trx_(trx)
  {}

  virtual ~UpdateOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override
  {
    return nullptr;
  }
  int row_num()
  {
    return row_num_;
  }

private:
  int row_num_ = 0;
  UpdateStmt *update_stmt_ = nullptr;
  Trx *trx_ = nullptr;
};