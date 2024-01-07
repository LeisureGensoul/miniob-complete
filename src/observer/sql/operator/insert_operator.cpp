/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/insert_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/common/table.h"
#include "rc.h"

RC InsertOperator::open()
{
  RC rc = RC::SUCCESS;
  Table *table = insert_stmt_->table();
  // const Value *values = insert_stmt_->values();
  int row_amount = insert_stmt_->row_amount();  // 获取插入的行数
  int value_amount = insert_stmt_->value_amount();  // 获取每行的值的数量
  // return table->insert_record(nullptr, value_amount, values); // TODO trx
  
  // 如果插入的行数大于1，则调用表的插入多行记录方法
  if (row_amount > 1) {
    rc = table->insert_record(trx_, value_amount, insert_stmt_->rows());
  } else {
    // 否则，调用表的插入单行记录方法
    rc = table->insert_record(trx_, value_amount, insert_stmt_->rows()->at(0).values);
  }

  // 处理插入操作的结果
  if (RC::SUCCESS != rc) {
    LOG_WARN("insert record failed");
  }
  return rc;
}

RC InsertOperator::next()
{
  return RC::RECORD_EOF;
}

RC InsertOperator::close()
{
  return RC::SUCCESS;
}
