#include "sql/operator/join_operator.h"

RC JoinOperator::open()
{
  // 初始化返回码
  RC rc = RC::SUCCESS;

  // 打开左表和右表
  if (RC::SUCCESS != (rc = left_->open())) {
    rc = RC::INTERNAL;
    LOG_WARN("JoinOperater 左子操作符打开失败!");
  }
  if (RC::SUCCESS != (rc = right_->open())) {
    rc = RC::INTERNAL;
    LOG_WARN("JoinOperater 右子操作符打开失败!");
  }

  // 初始化元组，使用左右表的当前元组
  Tuple *left_tuple = left_->current_tuple();
  Tuple *right_tuple = right_->current_tuple();
  tuple_.init(left_tuple, right_tuple);

  return rc;
}


//void JoinOperator::filter_right_table()

RC JoinOperator::fetch_right_table()
{
  // 初始化返回码
  RC rc = RC::SUCCESS;

  // 从右表获取记录
  while (RC::SUCCESS == (rc = right_->next())) {
    // 存储右表的记录
    CompoundRecord cpd_rcd;
    right_->current_tuple()->get_record(cpd_rcd);

    // 深拷贝记录并加入右表记录列表
    for (auto &rcd_ptr : cpd_rcd) {
      rcd_ptr = new Record(*rcd_ptr);
    }
    rht_.emplace_back(cpd_rcd);
  }

  // 初始化右表记录迭代器
  rht_it_ = rht_.begin();
  print_info();

  if (RC::RECORD_EOF == rc) {
    return RC::SUCCESS;
  }

  return rc;
}

void JoinOperator::print_info()
{
  std::cout << "右表信息: " << std::endl;
  std::cout << "当前右表迭代器: " << std::endl;
  std::cout << rht_it_ - rht_.begin() << std::endl;
  std::cout << "当前元组: " << std::endl;
  //
}

RC JoinOperator::next()
{
  // 初始化返回码
  RC rc = RC::SUCCESS;

  // 如果是第一次执行next
  if (is_first_) {
    // 获取左表的下一个元组
    rc = left_->next();
    is_first_ = false;
    if (RC::SUCCESS != rc) {
      return rc;
    }

    // 获取右表的所有记录
    rc = fetch_right_table();
    if (RC::SUCCESS != rc) {
      return rc;
    }
    assert(rht_.begin() == rht_it_);
  }

  // 如果右表记录列表中还有记录
  if (rht_.end() != rht_it_) {
    // 设置当前元组的右表记录
    CompoundRecord temp(*rht_it_);
    tuple_.set_right_record(temp);
    rht_it_++;
    return RC::SUCCESS;
  }

  // 如果右表记录列表为空，则获取左表的下一个元组
  rc = left_->next();
  if (RC::SUCCESS == rc) {
    rht_it_ = rht_.begin();
    return next();
  }

  // LOG_ERROR
  return rc;
}

RC JoinOperator::close()
{
  // 初始化返回码
  RC rc = RC::SUCCESS;

  // 关闭左表和右表
  left_->close();
  right_->close();

  return rc;
}

Tuple *JoinOperator::current_tuple()
{
  // 返回当前元组
  return &tuple_;
}
