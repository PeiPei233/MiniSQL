//
// Created by njz on 2023/1/27.
//

#include "executor/executors/insert_executor.h"

InsertExecutor::InsertExecutor(ExecuteContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void InsertExecutor::Init() {
  child_executor_->Init();
}

bool InsertExecutor::Next([[maybe_unused]] Row *row, RowId *rid) {
  Row child_row;
  RowId child_rid;
  if (child_executor_->Next(&child_row, &child_rid)) {
    TableInfo *table_info = nullptr;
    dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
    if (res != DB_SUCCESS) {
      ASSERT(res != DB_SUCCESS, "Table not found");
      // throw std::runtime_error("Table not found");
    }
    // insert into indexes of the table first
    std::vector<IndexInfo *> indexes;
    exec_ctx_->GetCatalog()->GetTableIndexes(plan_->GetTableName(), indexes);
    for (auto index : indexes) {
      res = index->GetIndex()->InsertEntry(child_row, child_rid, exec_ctx_->GetTransaction());
      if (res != DB_SUCCESS) {
        ASSERT(res != DB_SUCCESS, "duplicate key");
        // throw std::runtime_error("duplicate key");
      }
    }
    // insert into the table
    table_info->GetTableHeap()->InsertTuple(child_row, exec_ctx_->GetTransaction());
    return true;
  }
  return false;
}