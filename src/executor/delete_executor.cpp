//
// Created by njz on 2023/1/29.
//

#include "executor/executors/delete_executor.h"

/**
* TODO: Student Implement
*/

DeleteExecutor::DeleteExecutor(ExecuteContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  child_executor_->Init();
}

bool DeleteExecutor::Next([[maybe_unused]] Row *row, RowId *rid) {
  Row child_row;
  RowId child_rid;
  TableInfo *table_info = nullptr;
  dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
  if (res != DB_SUCCESS) {
    ASSERT(res != DB_SUCCESS, "Table not found");
    // throw std::runtime_error("Table not found");
  }
  if (child_executor_->Next(&child_row, &child_rid)) {
    // update indexes of the table first
    std::vector<IndexInfo *> indexes;
    exec_ctx_->GetCatalog()->GetTableIndexes(plan_->GetTableName(), indexes);
    for (auto index : indexes) {
      Row index_row;
      child_row.GetKeyFromRow(table_info->GetSchema(), index->GetIndexKeySchema(), index_row);
      std::cout << "index row field count: " << index_row.GetFieldCount() << " index schema field count: " << index->GetIndexKeySchema()->GetColumnCount() << std::endl;
      res = index->GetIndex()->RemoveEntry(index_row, child_rid, exec_ctx_->GetTransaction());
    }
    // update the table
    table_info->GetTableHeap()->ApplyDelete(child_rid, exec_ctx_->GetTransaction());
    return true;
  }
  return false;
}