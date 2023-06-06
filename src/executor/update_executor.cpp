//
// Created by njz on 2023/1/30.
//

#include "executor/executors/update_executor.h"

UpdateExecutor::UpdateExecutor(ExecuteContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

/**
* TODO: Student Implement
*/
void UpdateExecutor::Init() {
  // LOG(INFO) << "UpdateExecutor::Init";
  child_executor_->Init();
}

bool UpdateExecutor::Next([[maybe_unused]] Row *row, RowId *rid) {
  // LOG(INFO) << "UpdateExecutor::Next";
  Row child_row;
  RowId child_rid;
  if (child_executor_->Next(&child_row, &child_rid)) {
    TableInfo *table_info = nullptr;
    dberr_t res = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableName(), table_info);
    if (res != DB_SUCCESS) {
      ASSERT(res != DB_SUCCESS, "Table not found");
      // throw std::runtime_error("Table not found");
    }
    // update indexes of the table first
    std::vector<IndexInfo *> indexes;
    exec_ctx_->GetCatalog()->GetTableIndexes(plan_->GetTableName(), indexes);
    // LOG(INFO) << "indexes size: " << indexes.size();
    for (auto &index : indexes) {
      Row index_row;
      child_row.GetKeyFromRow(table_info->GetSchema(), index->GetIndexKeySchema(), index_row);
      // std::cout << "index row field count: " << index_row.GetFieldCount() << " index schema field count: " << index->GetIndexKeySchema()->GetColumnCount() << std::endl;
      res = index->GetIndex()->RemoveEntry(index_row, child_rid, exec_ctx_->GetTransaction());
    }
    // update the table
    Row updated_row = GenerateUpdatedTuple(child_row);
    table_info->GetTableHeap()->UpdateTuple(updated_row, child_rid, exec_ctx_->GetTransaction());
    // update indexes of the table
    for (auto index : indexes) {
      res = index->GetIndex()->InsertEntry(updated_row, child_rid, exec_ctx_->GetTransaction());
      if (res != DB_SUCCESS) {
        ASSERT(res != DB_SUCCESS, "duplicate key");
        // throw std::runtime_error("duplicate key");
      }
    }
    return true;
  }
  return false;
}

Row UpdateExecutor::GenerateUpdatedTuple(const Row &src_row) {
  // LOG(INFO) << "UpdateExecutor::GenerateUpdatedTuple";
  auto update_attrs = plan_->GetUpdateAttr(); /** Map from column index -> update operation */
  std::vector<Field> values;
  for (int i = 0; i < src_row.GetFieldCount(); i++) {
    if (update_attrs.find(i) != update_attrs.end()) {
      values.emplace_back(update_attrs[i]->Evaluate(&src_row));
    } else {
      values.emplace_back(*(src_row.GetField(i)));
    }
  }
  return Row{values};  
}