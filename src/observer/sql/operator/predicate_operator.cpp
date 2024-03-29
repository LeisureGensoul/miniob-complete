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
// Created by WangYunlai on 2022/6/27.
//

#include "common/lang/defer.h"
#include "common/log/log.h"
#include "sql/expr/expression.h"
#include "sql/operator/predicate_operator.h"
#include "sql/parser/parse_defs.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"
#include "util/typecast.h"
#include <stdlib.h>
#include <regex>
#include <string>

RC PredicateOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("predicate operator must has one child");
    return RC::INTERNAL;
  }

  return children_[0]->open();
}

RC PredicateOperator::next()
{
  RC rc = RC::SUCCESS;
  Operator *oper = children_[0];
  
  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }

    // if (do_predicate(static_cast<RowTuple &>(*tuple))) {
    //   return rc;
    // }
    bool res = false;
    CompoundTuple cpd_tup(tuple, const_cast<Tuple *>(parent_tuple_));
    if (nullptr == parent_tuple_) {
      rc = do_predicate(*tuple, res);
    } else {
      rc = do_predicate(cpd_tup, res);
    }
    if (RC::SUCCESS != rc) {
      return rc;
    }
    if (res) {
      return rc;
    }
  }
  return rc;
}

RC PredicateOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

Tuple * PredicateOperator::current_tuple()
{
  return children_[0]->current_tuple();
}

static void replace_all(std::string &str, const std::string &from, const std::string &to)
{
  if (from.empty()) {
    return;
  }
  size_t pos = 0;
  while (std::string::npos != (pos = str.find(from, pos))) {
    str.replace(pos, from.length(), to);
    pos += to.length();  // in case 'to' contains 'from'
  }
}

// bool PredicateOperator::do_predicate(RowTuple &tuple)
RC PredicateOperator::do_predicate(Tuple &tuple, bool &res)
{
  if (filter_stmt_ == nullptr || filter_stmt_->filter_units().empty()) {
    res = true;
    return RC::SUCCESS;
  }

  // the relation between two filter_units is AND
  for (const FilterUnit *filter_unit : filter_stmt_->filter_units()) {
    Expression *left_expr = filter_unit->left();
    Expression *right_expr = filter_unit->right();
    CompOp comp = filter_unit->comp();
    TupleCell left_cell;
    TupleCell right_cell;

    // 0. for [not] exist
    if (CompOp::EXISTS_OP == comp || CompOp::NOT_EXISTS == comp) {
      // TODO(wbj)
      // assert(nullptr == left_expr);
      assert(ExprType::SUBQUERYTYPE == right_expr->type());
      auto sub_query_expr = (const SubQueryExpression *)right_expr;
      sub_query_expr->open_sub_query();
      // TODO compound with parent tuple
      RC tmp_rc = sub_query_expr->get_value(tuple, right_cell);
      sub_query_expr->close_sub_query();
      res = CompOp::EXISTS_OP == comp ? (RC::SUCCESS == tmp_rc) : (RC::RECORD_EOF == tmp_rc);
      if (!res) {
        return RC::SUCCESS;
      }
      continue;
    }

    // 1. for [not] in
    if (CompOp::IN_OP == comp || CompOp::NOT_IN == comp) {
      left_expr->get_value(tuple, left_cell);
      if (left_cell.is_null()) {
        res = false;  // null don't in/not in any list
        return RC::SUCCESS;
      }
      std::vector<TupleCell> right_cells;
      right_cells.emplace_back(TupleCell());
      RC rc = RC::SUCCESS;
      if (ExprType::SUBQUERYTYPE == right_expr->type()) {
        auto sub_query_expr = (const SubQueryExpression *)right_expr;
        sub_query_expr->open_sub_query();
        while (RC::SUCCESS == (rc = sub_query_expr->get_value(tuple, right_cells.back()))) {
          right_cells.emplace_back(TupleCell());
        }
        sub_query_expr->close_sub_query();
        assert(RC::RECORD_EOF == rc);
        right_cells.pop_back();  // pop null cell for record_eof
      } else {
        assert(ExprType::SUBLISTTYPE == right_expr->type());
        auto list_expr = (const ListExpression *)right_expr;
        right_cells = list_expr->get_tuple_cells();
      }

      auto has_null = [](const std::vector<TupleCell> &cells) {
        for (auto &cell : cells) {
          if (cell.is_null()) {
            return true;
          }
        }
        return false;
      };
      res = CompOp::IN_OP == comp ? left_cell.in_cells(right_cells)
                                  : (has_null(right_cells) ? false : left_cell.not_in_cells(right_cells));
      if (!res) {
        return RC::SUCCESS;
      }
      continue;
    }

    auto get_cell_for_sub_query = [](const SubQueryExpression *expr, const Tuple &tuple, TupleCell &cell) {
      expr->open_sub_query();
      RC rc = expr->get_value(tuple, cell);
      if (RC::RECORD_EOF == rc) {
        // e.g. a = select a  -> a = null
        cell.set_null();
      } else if (RC::SUCCESS == rc) {
        TupleCell tmp_cell;
        if (RC::SUCCESS == (rc = expr->get_value(tuple, tmp_cell))) {
          // e.g. a = select a  -> a = (1, 2, 3)
          // std::cout << "Should not have rows more than 1" << std::endl;
          expr->close_sub_query();
          return RC::INTERNAL;
        }
      } else {
        expr->close_sub_query();
        return rc;
      }
      expr->close_sub_query();
      return RC::SUCCESS;
    };

    // std::cout << "DO PREDICATE: comp : " << comp << std::endl;

    if (ExprType::SUBQUERYTYPE == left_expr->type()) {
      RC rc = get_cell_for_sub_query((const SubQueryExpression *)left_expr, tuple, left_cell);
      if (RC::SUCCESS != rc) {
        return rc;
      }
    } else {
      left_expr->get_value(tuple, left_cell);
    }
    // std::cout << "DO PREDICATE: get_left_cell : ";
    // left_cell.to_string(std::cout);
    // std::cout << std::endl;

    if (ExprType::SUBQUERYTYPE == right_expr->type()) {
      RC rc = get_cell_for_sub_query((const SubQueryExpression *)right_expr, tuple, right_cell);
      if (RC::SUCCESS != rc) {
        return rc;
      }
    } else {
      right_expr->get_value(tuple, right_cell);
    }
    // std::cout << "DO PREDICATE: get_right_cell : ";
    // right_cell.to_string(std::cout);
    // std::cout << std::endl;


    // 0. for is [not] null
    if (CompOp::IS_NULL == comp) {
      assert(right_cell.is_null());
      res = left_cell.is_null();
      if (!res) {
        return RC::SUCCESS;
      }
      continue;
    }
    if (CompOp::IS_NOT_NULL == comp) {
      assert(right_cell.is_null());
      res = !left_cell.is_null();
      if (!res) {
        return RC::SUCCESS;
      }
      continue;
    }

    // 1. check null
    if (left_cell.is_null() || right_cell.is_null()) {
      res = false;
      return RC::SUCCESS;
    }

    AttrType left_type = left_cell.attr_type();
    AttrType right_type = right_cell.attr_type();

    // some predicates. contains like, in, compare.
    // 2. for like. only occur in chars type. no need to concern typecast
    if (LIKE_OP == comp || NOT_LIKE_OP == comp) {
      assert(CHARS == left_type && CHARS == right_type);
      std::string raw_reg((const char *)right_cell.data());
      replace_all(raw_reg, "_", "[^']");
      replace_all(raw_reg, "%", "[^']*");
      std::regex reg(raw_reg.c_str(), std::regex_constants::ECMAScript | std::regex_constants::icase);
      bool res = std::regex_match((const char *)left_cell.data(), reg);
      // std::cout << (const char *)left_cell.data() << " " << raw_reg << " " << res << std::endl;
      if ((LIKE_OP == comp && !res) || (NOT_LIKE_OP == comp && res)) {
        res = false;
        return RC::SUCCESS;
      }
      continue;
    }

    // 3. for compare: > >= < <= == != <>
    
    const int compare = left_cell.compare(right_cell);
    bool filter_result = false;
    switch (comp) {
    case EQUAL_TO: {
      filter_result = (0 == compare); 
    } break;
    case LESS_EQUAL: {
      filter_result = (compare <= 0); 
    } break;
    case NOT_EQUAL: {
      filter_result = (compare != 0);
    } break;
    case LESS_THAN: {
      filter_result = (compare < 0);
    } break;
    case GREAT_EQUAL: {
      filter_result = (compare >= 0);
    } break;
    case GREAT_THAN: {
      filter_result = (compare > 0);
    } break;
    default: {
      LOG_WARN("invalid compare type: %d", comp);
    } break;
    }
    if (!filter_result) {
      res = false;
      return RC::SUCCESS;
    }
  }
  res = true;
  return RC::SUCCESS;
}

// int PredicateOperator::tuple_cell_num() const
// {
//   return children_[0]->tuple_cell_num();
// }
// RC PredicateOperator::tuple_cell_spec_at(int index, TupleCellSpec &spec) const
// {
//   return children_[0]->tuple_cell_spec_at(index, spec);
// }

// ??
// RC PredicateOperator::do_predicate(Tuple &tuple, bool &res)
// {
//   return do_predicate(filter_stmt_->filter_units(), tuple, res);
// }