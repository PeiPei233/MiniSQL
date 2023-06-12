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
    std::vector<Row> index_rows;
    for (auto &index : indexes) {
      Row index_row;
      child_row.GetKeyFromRow(table_info->GetSchema(), index->GetIndexKeySchema(), index_row);
      // std::cout << "index row field count: " << index_row.GetFieldCount() << " index schema field count: " << index->GetIndexKeySchema()->GetColumnCount() << std::endl;
      res = index->GetIndex()->RemoveEntry(index_row, child_rid, exec_ctx_->GetTransaction());
      ASSERT(res == DB_SUCCESS, "Remove entry failed");
      index_rows.emplace_back(index_row);
    }
    // generate updated tuple
    Row updated_row = GenerateUpdatedTuple(child_row);
    // check for duplicate
    for (auto index : indexes) {
      Row index_row;
      updated_row.GetKeyFromRow(table_info->GetSchema(), index->GetIndexKeySchema(), index_row);
      std::vector<RowId> rids;
      res = index->GetIndex()->ScanKey(index_row, rids, exec_ctx_->GetTransaction());
      if (rids.size() > 0) {
        // roll back the deleted entries
        for (int i = 0; i < indexes.size(); i++) {
          res = indexes[i]->GetIndex()->InsertEntry(index_rows[i], child_rid, exec_ctx_->GetTransaction());
          ASSERT(res == DB_SUCCESS, "Insert entry failed");
        }
        throw std::runtime_error("Duplicate key");
        return false;
      }
      ASSERT(res != DB_SUCCESS, "Duplicate key");
    }
    // update indexes of the table
    for (auto index : indexes) {
      Row index_row;
      updated_row.GetKeyFromRow(table_info->GetSchema(), index->GetIndexKeySchema(), index_row);
      res = index->GetIndex()->InsertEntry(index_row, child_rid, exec_ctx_->GetTransaction());
      ASSERT(res == DB_SUCCESS, "Insert entry failed");
    }
    // update the table
    if(table_info->GetTableHeap()->UpdateTuple(updated_row, child_rid, exec_ctx_->GetTransaction())<=1)
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