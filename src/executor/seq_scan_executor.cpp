//
// Created by njz on 2023/1/17.
//
#include "executor/executors/seq_scan_executor.h"

/**
* TODO: Student Implement
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
  std::cout << "SeqScanExecutor::Init()" << std::endl;
}

bool SeqScanExecutor::Next(Row *row, RowId *rid) {
  std::cout << "SeqScanExecutor::Next()" << std::endl;
  TableInfo *table_info = nullptr;
  dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
  if (res != DB_SUCCESS) {
    throw std::runtime_error("Table not found");
  }
  auto end = table_info->GetTableHeap()->End();
  while (iter_ == end) {
    std::cout << (*iter_).GetFieldCount() << std::endl;
    if (plan_->GetPredicate() == nullptr || plan_->GetPredicate()->Evaluate(&*iter_).CompareEquals(Field(kTypeInt, 1)) == CmpBool::kTrue) {
      *row = *iter_;
      *rid = row->GetRowId();
      ++iter_;
      return true;
    }
    ++iter_;
  }
  return false;
}
