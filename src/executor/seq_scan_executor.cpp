//
// Created by njz on 2023/1/17.
//
#include "executor/executors/seq_scan_executor.h"

/**
* Student Implement
*/
SeqScanExecutor::SeqScanExecutor(ExecuteContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx),
      plan_(plan){}

void SeqScanExecutor::Init() {
  TableInfo *table_info = nullptr;
  dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
  if (res != DB_SUCCESS) {
    throw std::runtime_error("Table not found");
  }
  iter_ = table_info->GetTableHeap()->Begin(exec_ctx_->GetTransaction());
  // LOG(INFO) << "SeqScanExecutor::Init()";
}

bool SeqScanExecutor::Next(Row *row, RowId *rid) {
  // LOG(INFO) << "SeqScanExecutor::Next()";
  TableInfo *table_info = nullptr;
  dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
  if (res != DB_SUCCESS) {
    throw std::runtime_error("Table not found");
  }
  auto end = table_info->GetTableHeap()->End();
  while (iter_ != end) {
    // std::cout << "iter row page id " << (*iter_).GetRowId().GetPageId() << std::endl;
    // std::cout << "iter field count " << (*iter_).GetFieldCount() << std::endl;
    if (plan_->GetPredicate() == nullptr || plan_->GetPredicate()->Evaluate(&*iter_).CompareEquals(Field(kTypeInt, 1)) == CmpBool::kTrue) {
      Row n_row(*iter_);
      n_row.GetKeyFromRow(table_info->GetSchema(), plan_->OutputSchema(), *row);
      *rid = row->GetRowId();
      ++iter_;
      return true;
    }
    ++iter_;
  }
  return false;
}
