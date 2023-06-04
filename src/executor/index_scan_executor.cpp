#include "executor/executors/index_scan_executor.h"
/**
* TODO: Student Implement
*/
IndexScanExecutor::IndexScanExecutor(ExecuteContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
  for (auto index_iter = plan_->indexes_.begin(); index_iter != plan_->indexes_.end(); ++index_iter) {
    std::vector<RowId> row_ids_n;
    auto index_info = *index_iter;
    auto index = static_cast<BPlusTreeIndex *>(index_info->GetIndex());
    for (auto iter = index->GetBeginIterator(); iter != index->GetEndIterator(); ++iter) {
      row_ids_n.emplace_back((*iter).second);
    }
    if (index_iter == plan_->indexes_.begin()) {
      row_ids_ = row_ids_n;
    } else {
      std::vector<RowId> row_ids_t;
      std::sort(row_ids_.begin(), row_ids_.end(), 
                [](const RowId &a, const RowId &b) {
                  return a.GetPageId() < b.GetPageId() || (a.GetPageId() == b.GetPageId() && a.GetSlotNum() < b.GetSlotNum());
                });
      std::sort(row_ids_n.begin(), row_ids_n.end(), 
                [](const RowId &a, const RowId &b) {
                  return a.GetPageId() < b.GetPageId() || (a.GetPageId() == b.GetPageId() && a.GetSlotNum() < b.GetSlotNum());
                });
      std::set_intersection(row_ids_.begin(), row_ids_.end(), 
                            row_ids_n.begin(), row_ids_n.end(), 
                            std::back_inserter(row_ids_t),
                            [](const RowId &a, const RowId &b) {
                              return a.GetPageId() < b.GetPageId() || (a.GetPageId() == b.GetPageId() && a.GetSlotNum() < b.GetSlotNum());
                            });
      row_ids_ = row_ids_t;
    }
  }
  iter_ = row_ids_.begin();
}

bool IndexScanExecutor::Next(Row *row, RowId *rid) {
  while (iter_ != row_ids_.end()) {
    TableInfo *table_info = nullptr;
    dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
    if (res != DB_SUCCESS) {
      ASSERT(res != DB_SUCCESS, "Table not found");
      // throw std::runtime_error("Table not found");
    }
    row = new Row(*iter_);
    table_info->GetTableHeap()->GetTuple(row, exec_ctx_->GetTransaction());
    if (plan_->GetPredicate() == nullptr || plan_->GetPredicate()->Evaluate(row).CompareEquals(Field(kTypeInt, 1)) == CmpBool::kTrue) {
      *rid = *iter_;
      ++iter_;
      return true;
    }
    ++iter_;
  }
  return false;
}
