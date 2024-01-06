#include "common/log/log.h"
#include "sql/operator/update_operator.h"
#include "storage/record/record.h"
#include "storage/common/table.h"
#include "sql/stmt/update_stmt.h"

// 打开更新操作符
RC UpdateOperator::open()
{
  // 初始化返回码
  RC rc = RC::SUCCESS;

  // 确保更新操作符有一个子操作符
  if (children_.size() != 1) {
    LOG_WARN("更新操作符必须有一个子操作符");
    return RC::INTERNAL;
  }

  // 获取子操作符并打开
  Operator *child = children_[0];
  rc = child->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("无法打开子操作符: %s", strrc(rc));
    return rc;
  }

  // 获取更新语句对应的表
  Table *table = update_stmt_->table();

  // 循环遍历子操作符的结果集
  while (RC::SUCCESS == (rc = child->next())) {
    // 获取当前元组
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("无法获取当前记录。返回码=%s", strrc(rc));
      break;
    }

    // 将元组转换为行元组
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);

    // 获取行元组对应的记录
    Record &record = row_tuple->record();

    // 使用更新语句更新表中的记录
    rc = table->update_record(trx_, update_stmt_->attr_name(), &record, update_stmt_->values());
    if (rc != RC::SUCCESS) {
      // 如果更新失败，检查是否为重复键的情况
      if (rc == RC::RECORD_DUPLICATE_KEY) {
        LOG_WARN("相同的值，无需更新");
        continue;
      } else {
        LOG_WARN("更新记录失败: %s", strrc(rc));
        return rc;
      }
    }

    // 成功更新一行
    row_num_++;
  }

  // 检查循环结束时的返回码
  if (RC::RECORD_EOF != rc) {
    LOG_WARN("更新失败！");
    return rc;
  }

  return RC::SUCCESS;
}

// 获取下一个元组（此操作符无需实现）
RC UpdateOperator::next()
{
  return RC::RECORD_EOF;
}

// 关闭更新操作符
RC UpdateOperator::close()
{
  // 关闭子操作符
  children_[0]->close();
  return RC::SUCCESS;
}
