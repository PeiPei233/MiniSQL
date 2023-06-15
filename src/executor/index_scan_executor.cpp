#include "executor/executors/index_scan_executor.h"
/**
* Student Implement
*/
IndexScanExecutor::IndexScanExecutor(ExecuteContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::UpdateRowIdsFromCompExp(const ComparisonExpression *comp) {
  auto col = reinterpret_cast<const ColumnValueExpression *>(comp->GetChildAt(0).get());
  auto val = reinterpret_cast<const ConstantValueExpression *>(comp->GetChildAt(1).get());
  auto col_idx = col->GetColIdx();
  Field val_field = val->Evaluate(nullptr);
  std::vector<Field> fields;
  fields.push_back(val_field);
  Row row(fields);
  std::vector<RowId> row_ids;
  bool has_index = false;
  for (auto index_info : plan_->indexes_) {
    if (index_info->GetIndexKeySchema()->GetColumnCount() == 1 &&
        index_info->GetIndexKeySchema()->GetColumn(0)->GetTableInd() == col_idx) {
      index_info->GetIndex()->ScanKey(row, row_ids, nullptr, comp->GetComparisonType());
      has_index = true;
      break;
    }
  }
  if (!has_index) {
    if (!plan_->need_filter_) {
      throw std::runtime_error("No index found");
    }
    return;
  }
  std::sort(row_ids.begin(), row_ids.end(), [](const RowId &a, const RowId &b) {
    return a.GetPageId() < b.GetPageId() || (a.GetPageId() == b.GetPageId() && a.GetSlotNum() < b.GetSlotNum());
  });
  if (row_ids_.empty()) {
    row_ids_ = row_ids;
  } else {
    std::vector<RowId> new_row_ids;
    std::set_intersection(row_ids_.begin(), row_ids_.end(), row_ids.begin(), row_ids.end(),
                          std::back_inserter(new_row_ids), [](const RowId &a, const RowId &b) {
                            return a.GetPageId() < b.GetPageId() ||
                                   (a.GetPageId() == b.GetPageId() && a.GetSlotNum() < b.GetSlotNum());
                          });
    row_ids_ = new_row_ids;
  }
}

void IndexScanExecutor::Init() {
  TableInfo *table_info = nullptr;
  dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
  if (res != DB_SUCCESS) {
    ASSERT(res == DB_SUCCESS, "Table not found");
    throw std::runtime_error("Table not found");
  }
  if (plan_->GetPredicate() == nullptr) {
    throw std::runtime_error("Predicate is null");
  }
  if (plan_->GetPredicate()->GetType() == ExpressionType::ComparisonExpression) {
    auto comp = reinterpret_cast<const ComparisonExpression *>(plan_->GetPredicate().get());
    UpdateRowIdsFromCompExp(comp);
  } else {
    if (plan_->GetPredicate()->GetType() != ExpressionType::LogicExpression) {
      throw std::runtime_error("Predicate is not a logic expression");
    }
    auto cur = reinterpret_cast<const LogicExpression *>(plan_->GetPredicate().get());
    while (cur->GetChildAt(0)->GetType() == ExpressionType::LogicExpression) {
      auto comp = reinterpret_cast<const ComparisonExpression *>(cur->GetChildAt(1).get());
      UpdateRowIdsFromCompExp(comp);
      cur = reinterpret_cast<const LogicExpression *>(cur->GetChildAt(0).get());
    }
    auto comp = reinterpret_cast<const ComparisonExpression *>(cur->GetChildAt(0).get());
    UpdateRowIdsFromCompExp(comp);
    comp = reinterpret_cast<const ComparisonExpression *>(cur->GetChildAt(1).get());
    UpdateRowIdsFromCompExp(comp);
  }
  iter_ = row_ids_.begin();
}

bool IndexScanExecutor::Next(Row *row, RowId *rid) {
  while (iter_ != row_ids_.end()) {
    TableInfo *table_info = nullptr;
    dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
    if (res != DB_SUCCESS) {
      ASSERT(res == DB_SUCCESS, "Table not found");
      throw std::runtime_error("Table not found");
    }
    auto n_row = new Row(*iter_);
    table_info->GetTableHeap()->GetTuple(n_row, exec_ctx_->GetTransaction());
    if (!plan_->need_filter_ || plan_->GetPredicate()->Evaluate(n_row).CompareEquals(Field(kTypeInt, 1)) == CmpBool::kTrue) {
      *rid = *iter_;
      n_row->GetKeyFromRow(table_info->GetSchema(), plan_->OutputSchema(), *row);
      ++iter_;
      delete n_row;
      return true;
    }
    delete n_row;
    ++iter_;
  }
  return false;
}
