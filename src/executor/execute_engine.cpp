#include "executor/execute_engine.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>

#include "common/result_writer.h"
#include "executor/executors/delete_executor.h"
#include "executor/executors/index_scan_executor.h"
#include "executor/executors/insert_executor.h"
#include "executor/executors/seq_scan_executor.h"
#include "executor/executors/update_executor.h"
#include "executor/executors/values_executor.h"
#include "glog/logging.h"
#include "planner/planner.h"
#include "utils/utils.h"

extern "C" {
int yyparse(void);
#include "parser/minisql_lex.h"
#include "parser/parser.h"
}

ExecuteEngine::ExecuteEngine() {
  char path[] = "./databases";
  DIR *dir;
  if((dir = opendir(path)) == nullptr) {
    mkdir("./databases", 0777);
    dir = opendir(path);
  }
  /** When you have completed all the code for
   *  the test, run it using main.cpp and uncomment
   *  this part of the code.*/
  struct dirent *stdir;
  while((stdir = readdir(dir)) != nullptr) {
    if( strcmp( stdir->d_name , "." ) == 0 ||
        strcmp( stdir->d_name , "..") == 0 ||
        stdir->d_name[0] == '.')
      continue;
    dbs_[stdir->d_name] = new DBStorageEngine(stdir->d_name, false);
  }
  closedir(dir);
}

std::unique_ptr<AbstractExecutor> ExecuteEngine::CreateExecutor(ExecuteContext *exec_ctx,
                                                                const AbstractPlanNodeRef &plan) {
  switch (plan->GetType()) {
    // Create a new sequential scan executor
    case PlanType::SeqScan: {
      return std::make_unique<SeqScanExecutor>(exec_ctx, dynamic_cast<const SeqScanPlanNode *>(plan.get()));
    }
    // Create a new index scan executor
    case PlanType::IndexScan: {
      return std::make_unique<IndexScanExecutor>(exec_ctx, dynamic_cast<const IndexScanPlanNode *>(plan.get()));
    }
    // Create a new update executor
    case PlanType::Update: {
      auto update_plan = dynamic_cast<const UpdatePlanNode *>(plan.get());
      auto child_executor = CreateExecutor(exec_ctx, update_plan->GetChildPlan());
      return std::make_unique<UpdateExecutor>(exec_ctx, update_plan, std::move(child_executor));
    }
      // Create a new delete executor
    case PlanType::Delete: {
      auto delete_plan = dynamic_cast<const DeletePlanNode *>(plan.get());
      auto child_executor = CreateExecutor(exec_ctx, delete_plan->GetChildPlan());
      return std::make_unique<DeleteExecutor>(exec_ctx, delete_plan, std::move(child_executor));
    }
    case PlanType::Insert: {
      auto insert_plan = dynamic_cast<const InsertPlanNode *>(plan.get());
      auto child_executor = CreateExecutor(exec_ctx, insert_plan->GetChildPlan());
      return std::make_unique<InsertExecutor>(exec_ctx, insert_plan, std::move(child_executor));
    }
    case PlanType::Values: {
      return std::make_unique<ValuesExecutor>(exec_ctx, dynamic_cast<const ValuesPlanNode *>(plan.get()));
    }
    default:
      throw std::logic_error("Unsupported plan type.");
  }
}

dberr_t ExecuteEngine::ExecutePlan(const AbstractPlanNodeRef &plan, std::vector<Row> *result_set, Transaction *txn,
                                   ExecuteContext *exec_ctx) {
  // Construct the executor for the abstract plan node
  auto executor = CreateExecutor(exec_ctx, plan);

  try {
    executor->Init();
    RowId rid{};
    Row row{};
    while (executor->Next(&row, &rid)) {
      if (result_set != nullptr) {
        result_set->push_back(row);
      }
    }

  } catch (const exception &ex) {
    std::cout << "Error Encountered in Executor Execution: " << ex.what() << std::endl;
    if (result_set != nullptr) {
      result_set->clear();
    }
    return DB_FAILED;
  }
  return DB_SUCCESS;
}

dberr_t ExecuteEngine::Execute(pSyntaxNode ast) {
  if (ast == nullptr) {
    return DB_FAILED;
  }
  auto start_time = std::chrono::system_clock::now();
  unique_ptr<ExecuteContext> context(nullptr);
  if(!current_db_.empty())
    context = dbs_[current_db_]->MakeExecuteContext(nullptr);
  switch (ast->type_) {
    case kNodeCreateDB:
      return ExecuteCreateDatabase(ast, context.get());
    case kNodeDropDB:
      return ExecuteDropDatabase(ast, context.get());
    case kNodeShowDB:
      return ExecuteShowDatabases(ast, context.get());
    case kNodeUseDB:
      return ExecuteUseDatabase(ast, context.get());
    case kNodeShowTables:
      return ExecuteShowTables(ast, context.get());
    case kNodeCreateTable:
      return ExecuteCreateTable(ast, context.get());
    case kNodeDropTable:
      return ExecuteDropTable(ast, context.get());
    case kNodeShowIndexes:
      return ExecuteShowIndexes(ast, context.get());
    case kNodeCreateIndex:
      return ExecuteCreateIndex(ast, context.get());
    case kNodeDropIndex:
      return ExecuteDropIndex(ast, context.get());
    case kNodeTrxBegin:
      return ExecuteTrxBegin(ast, context.get());
    case kNodeTrxCommit:
      return ExecuteTrxCommit(ast, context.get());
    case kNodeTrxRollback:
      return ExecuteTrxRollback(ast, context.get());
    case kNodeExecFile:
      return ExecuteExecfile(ast, context.get());
    case kNodeQuit:
      return ExecuteQuit(ast, context.get());
    default:
      break;
  }

  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }

  // Plan the query.
  Planner planner(context.get());
  std::vector<Row> result_set{};
  try {
    planner.PlanQuery(ast);
    // Execute the query.
    ExecutePlan(planner.plan_, &result_set, nullptr, context.get());
  } catch (const exception &ex) {
    std::cout << "Error Encountered in Planner: " << ex.what() << std::endl;
    return DB_FAILED;
  }
  auto stop_time = std::chrono::system_clock::now();
  double duration_time =
      double((std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time)).count());
  // Return the result set as string.
  std::stringstream ss;
  ResultWriter writer(ss);

  if (planner.plan_->GetType() == PlanType::SeqScan || planner.plan_->GetType() == PlanType::IndexScan) {
    auto schema = planner.plan_->OutputSchema();
    auto num_of_columns = schema->GetColumnCount();
    if (!result_set.empty()) {
      // find the max width for each column
      vector<int> data_width(num_of_columns, 0);
      for (const auto &row : result_set) {
        for (uint32_t i = 0; i < num_of_columns; i++) {
          data_width[i] = max(data_width[i], int(row.GetField(i)->toString().size()));
        }
      }
      int k = 0;
      for (const auto &column : schema->GetColumns()) {
        data_width[k] = max(data_width[k], int(column->GetName().length()));
        k++;
      }
      // Generate header for the result set.
      writer.Divider(data_width);
      k = 0;
      writer.BeginRow();
      for (const auto &column : schema->GetColumns()) {
        writer.WriteHeaderCell(column->GetName(), data_width[k++]);
      }
      writer.EndRow();
      writer.Divider(data_width);

      // Transforming result set into strings.
      for (const auto &row : result_set) {
        writer.BeginRow();
        for (uint32_t i = 0; i < schema->GetColumnCount(); i++) {
          writer.WriteCell(row.GetField(i)->toString(), data_width[i]);
        }
        writer.EndRow();
      }
      writer.Divider(data_width);
    }
    writer.EndInformation(result_set.size(), duration_time, true);
  } else {
    writer.EndInformation(result_set.size(), duration_time, false);
  }
  std::cout << writer.stream_.rdbuf();
  return DB_SUCCESS;
}

void ExecuteEngine::ExecuteInformation(dberr_t result) {
  switch (result) {
    case DB_ALREADY_EXIST:
      cout << "Database already exists." << endl;
      break;
    case DB_NOT_EXIST:
      cout << "Database not exists." << endl;
      break;
    case DB_TABLE_ALREADY_EXIST:
      cout << "Table already exists." << endl;
      break;
    case DB_TABLE_NOT_EXIST:
      cout << "Table not exists." << endl;
      break;
    case DB_INDEX_ALREADY_EXIST:
      cout << "Index already exists." << endl;
      break;
    case DB_INDEX_NOT_FOUND:
      cout << "Index not exists." << endl;
      break;
    case DB_COLUMN_NAME_NOT_EXIST:
      cout << "Column not exists." << endl;
      break;
    case DB_KEY_NOT_FOUND:
      cout << "Key not exists." << endl;
      break;
    case DB_QUIT:
      cout << "Bye." << endl;
      break;
    default:
      break;
  }
}
/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteCreateDatabase(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteCreateDatabase" << std::endl;
#endif
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string db_name = ast->child_->val_;
  if (dbs_.find(db_name) != dbs_.end()) {
    return DB_ALREADY_EXIST;
  }
  auto db = new DBStorageEngine(db_name, true);
  dbs_.insert(std::make_pair(db_name, db));
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteDropDatabase(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteDropDatabase" << std::endl;
#endif
  std::string file_name = ast->child_->val_;
  file_name = std::string("./databases/") + file_name;
  std::string db_name = ast->child_->val_;
  if (dbs_.find(db_name) == dbs_.end()) {
    return DB_NOT_EXIST;
  }
  delete dbs_.at(db_name);
  dbs_.erase(db_name);
  if (current_db_ == db_name) {
    current_db_.clear();
  }
  if (remove(file_name.c_str()) != 0) {
    return DB_NOT_EXIST;
  }
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteShowDatabases(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteShowDatabases" << std::endl;
#endif
  auto start_time = std::chrono::high_resolution_clock::now();
  std::vector<std::string> db_names;
  std::vector<int> data_width {strlen("Database")};
  dirent *ent;
  for (auto &[db_name, db] : dbs_) {
    db_names.push_back(db_name);
    data_width[0] = max(data_width[0], int(db_name.length()));
  }
  if (!db_names.empty()) {
    std::stringstream ss;
    ResultWriter writer(ss);

    writer.Divider(data_width);
    writer.BeginRow();
    writer.WriteHeaderCell("Database", data_width[0]);
    writer.EndRow();
    writer.Divider(data_width);
    for (const auto &db_name : db_names) {
      writer.BeginRow();
      writer.WriteCell(db_name, data_width[0]);
      writer.EndRow();
    }
    writer.Divider(data_width);
    std::cout << writer.stream_.rdbuf();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << db_names.size() << " rows in set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  } else {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << "Empty set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  }

  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteUseDatabase(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteUseDatabase" << std::endl;
#endif
  std::string db_name = ast->child_->val_;
  if (dbs_.find(db_name) == dbs_.end()) {
    return DB_NOT_EXIST;
  }
  current_db_ = db_name;
  std::cout << "Database changed" << std::endl;
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteShowTables(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteShowTables" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::vector<std::string> table_names;
  std::vector<int> data_width {0};
  data_width[0] = max(data_width[0], int(strlen("Tables_in_") + current_db_.length()));
  if (current_db_.empty()) {
    return DB_NOT_EXIST;
  }
  auto db = dbs_.at(current_db_);
  std::vector<TableInfo *> tables;
  db->catalog_mgr_->GetTables(tables);
  for (auto &table : tables) {
    std::string table_name = table->GetTableName();
    table_names.push_back(table_name);
    data_width[0] = max(data_width[0], int(table_name.length()));
  }
  if (!table_names.empty()) {
    std::stringstream ss;
    ResultWriter writer(ss);

    writer.Divider(data_width);
    writer.BeginRow();
    writer.WriteHeaderCell("Tables_in_" + current_db_, data_width[0]);
    writer.EndRow();
    writer.Divider(data_width);
    for (const auto &table_name : table_names) {
      writer.BeginRow();
      writer.WriteCell(table_name, data_width[0]);
      writer.EndRow();
    }
    writer.Divider(data_width);
    std::cout << writer.stream_.rdbuf();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << table_names.size() << " rows in set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  } else {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << "Empty set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  }

  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteCreateTable(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteCreateTable" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string table_name = ast->child_->val_;
  std::vector<std::string> col_names;
  std::vector<TypeId> col_types;
  std::vector<int> col_lengths;
  std::vector<bool> col_uniques;
  std::vector<std::string> primary_keys;
  pSyntaxNode col_def = ast->child_->next_->child_;
  while (col_def != nullptr) {
    if (col_def->val_ != nullptr && std::string(col_def->val_) == std::string("primary keys")) {
      for (pSyntaxNode key = col_def->child_; key != nullptr; key = key->next_) {
        primary_keys.push_back(key->val_);
      }
    } else {
      std::string col_name;
      TypeId type = kTypeInvalid;
      int length = 0;
      if (col_def->val_ != nullptr && std::string(col_def->val_) == std::string("unique")) {
        col_uniques.push_back(true);
      } else {
        col_uniques.push_back(false);
      }
      for (pSyntaxNode col_attr = col_def->child_; col_attr != nullptr; col_attr = col_attr->next_) {
        if (col_attr->type_ == kNodeIdentifier) {
          col_name = col_attr->val_;
        } else if (col_attr->type_ == kNodeColumnType) {
          if (col_attr->val_ != nullptr && std::string(col_attr->val_) == std::string("int")) {
            type = kTypeInt;
          } else if (col_attr->val_ != nullptr && std::string(col_attr->val_) == std::string("float")) {
            type = kTypeFloat;
          } else if (col_attr->val_ != nullptr && std::string(col_attr->val_) == std::string("char")) {
            type = kTypeChar;
            ASSERT(col_attr->child_ != nullptr && col_attr->child_->val_ != nullptr, "Invalid column length");
            // LOG(INFO) << "col_attr->child_->val_ = " << col_attr->child_->val_ << std::endl;
            // return DB_FAILED when length < 0 or length is a floating number
            try {
              size_t pos;
              length = std::stoi(col_attr->child_->val_, &pos);
              if (pos != std::string(col_attr->child_->val_).length()) {
                std::cout << "Invalid column length" << std::endl;
                return DB_FAILED;
              }
            } catch (std::invalid_argument &e) {
              std::cout << "Invalid column length" << std::endl;
              return DB_FAILED;
            }
            if (length < 0) {
              std::cout << "Invalid column length" << std::endl;
              return DB_FAILED;
            }
          } else {
            std::cout << "Invalid column type" << std::endl;
            return DB_FAILED;
          }
        } else {
          std::cout << "Invalid column attribute" << std::endl;
          return DB_FAILED;
        }
      }
      col_names.push_back(col_name);
      col_types.push_back(type);
      col_lengths.push_back(length);
    }
    col_def = col_def->next_;
  }
  std::vector<Column *> columns;
  for (int i = 0; i < col_names.size(); i++) {
    bool nullable = true;
    if (std::find(primary_keys.begin(), primary_keys.end(), col_names[i]) != primary_keys.end()) {
      nullable = false;
    }
    if (col_types[i] == kTypeChar) {
      columns.push_back(new Column(col_names[i], col_types[i], col_lengths[i], i, nullable, col_uniques[i]));
    } else {
      columns.push_back(new Column(col_names[i], col_types[i], i, nullable, col_uniques[i]));
    }
  }
  TableSchema *schema = new TableSchema(columns);
  TableInfo *table_info = nullptr;
  dberr_t res = context->GetCatalog()->CreateTable(table_name, schema, nullptr, table_info);
  if (res != DB_SUCCESS) {
    return res;
  }
  // create index for unique columns
  for (int i = 0; i < col_names.size(); i++) {
    if (col_uniques[i]) {
      std::string index_name = table_name + "-" + col_names[i] + "-index";
      std::vector<std::string> index_keys {col_names[i]};
      IndexInfo *index_info = nullptr;
      res = context->GetCatalog()->CreateIndex(table_name, index_name, index_keys, nullptr, index_info, "bptree");
      if (res != DB_SUCCESS) {
        return res;
      }
    }
  }
  // create index for primary keys
  if (primary_keys.size() > 0) {
    std::string index_name = table_name + "-primary-keys-index";
    IndexInfo *index_info = nullptr;
    res = context->GetCatalog()->CreateIndex(table_name, index_name, primary_keys, nullptr, index_info, "bptree");
    if (res != DB_SUCCESS) {
      return res;
    }
  }
  delete schema;
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "Query OK, 0 rows affected (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteDropTable(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteDropTable" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string table_name = ast->child_->val_;
  // drop index
  std::vector<IndexInfo *> index_infos;
  dberr_t res = context->GetCatalog()->GetTableIndexes(table_name, index_infos);
  if (res != DB_SUCCESS) {
    return res;
  }
  for (auto index_info : index_infos) {
    res = context->GetCatalog()->DropIndex(table_name, index_info->GetIndexName());
    if (res != DB_SUCCESS) {
      return res;
    }
  }
  res = context->GetCatalog()->DropTable(table_name);
  if (res != DB_SUCCESS) {
    return res;
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "Query OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteShowIndexes(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteShowIndexes" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::vector<TableInfo *> table_infos;
  dberr_t res = context->GetCatalog()->GetTables(table_infos);
  if (res != DB_SUCCESS) {
    return res;
  }
  std::vector<IndexInfo *> index_infos;
  for (auto table_info : table_infos) {
    res = context->GetCatalog()->GetTableIndexes(table_info->GetTableName(), index_infos);
    if (res != DB_SUCCESS) {
      return res;
    }
  }
  std::vector<std::string> index_names;
  std::vector<int> data_width {static_cast<int>(strlen("Indexes_in_") + current_db_.length())};
  for (auto index_info : index_infos) {
    index_names.push_back(index_info->GetIndexName());
    data_width[0] = std::max(data_width[0], static_cast<int>(index_info->GetIndexName().length()));
  }
  if (!index_names.empty()) {
    std::stringstream ss;
    ResultWriter writer(ss);

    writer.Divider(data_width);
    writer.BeginRow();
    writer.WriteHeaderCell("Indexes_in_" + current_db_, data_width[0]);
    writer.EndRow();
    writer.Divider(data_width);
    for (const auto &index_name : index_names) {
      writer.BeginRow();
      writer.WriteCell(index_name, data_width[0]);
      writer.EndRow();
    }
    writer.Divider(data_width);
    std::cout << writer.stream_.rdbuf();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << index_names.size() << " rows in set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  } else {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    std::cout << "Empty set (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  }
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteCreateIndex(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteCreateIndex" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string index_name = ast->child_->val_;
  std::string table_name = ast->child_->next_->val_;
  pSyntaxNode col_n = ast->child_->next_->next_->child_;
  std::vector<std::string> column_names;
  while (col_n != nullptr) {
    column_names.push_back(col_n->val_);
    col_n = col_n->next_;
  }
  IndexInfo *index_info = nullptr;
  dberr_t res = context->GetCatalog()->CreateIndex(table_name, index_name, column_names, context->GetTransaction(), index_info, "bptree");
  if (res != DB_SUCCESS) {
    return res;
  }

  TableInfo *tableInfo= nullptr;
  context->GetCatalog()->GetTable(table_name,tableInfo);
  TableHeap *tableHeap=tableInfo->GetTableHeap();
  col_n = ast->child_->next_->next_->child_;
  std::vector<uint32_t >index_cols;
  index_cols.clear();
  while(col_n!= nullptr){
    uint32_t t_index;
    tableInfo->GetSchema()->GetColumnIndex(col_n->val_,t_index);
    index_cols.push_back(t_index);
    col_n = col_n->next_;
  }
  for(auto iter=tableHeap->Begin(nullptr);iter!=tableHeap->End();iter++){
    std::vector<Field> fields;
    for(auto iter2:index_cols){
      fields.push_back(*((*iter).GetField(iter2)));
    }
    Row index_row(fields);

    if(index_info->GetIndex()->InsertEntry(index_row,(*iter).GetRowId(), nullptr)==DB_FAILED){
      std::cout << "Duplicate entry" << std::endl;
      return DB_FAILED;
    }

  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "Query OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteDropIndex(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteDropIndex" << std::endl;
#endif
  if (context == nullptr) {
    std::cout << "No database selected." << std::endl;
    return DB_FAILED;
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string index_name = ast->child_->val_;
  std::string table_name {};
  std::vector<TableInfo *> table_infos;
  dberr_t res = context->GetCatalog()->GetTables(table_infos);
  if (res != DB_SUCCESS) {
    return res;
  }
  for (auto table_info : table_infos) {
    std::vector<IndexInfo *> index_infos;
    res = context->GetCatalog()->GetTableIndexes(table_info->GetTableName(), index_infos);
    if (res != DB_SUCCESS) {
      return res;
    }
    std::vector<std::string> index_names;
    for (auto index_info : index_infos) {
      index_names.push_back(index_info->GetIndexName());
    }
    if (std::find(index_names.begin(), index_names.end(), index_name) != index_names.end()) {
      table_name = table_info->GetTableName();
      break;
    }
  }
  if (table_name.empty()) {
    return DB_INDEX_NOT_FOUND;
  }
  res = context->GetCatalog()->DropIndex(table_name, index_name);
  if (res != DB_SUCCESS) {
    return res;
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "Query OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
}


dberr_t ExecuteEngine::ExecuteTrxBegin(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteTrxBegin" << std::endl;
#endif
  return DB_FAILED;
}

dberr_t ExecuteEngine::ExecuteTrxCommit(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteTrxCommit" << std::endl;
#endif
  return DB_FAILED;
}

dberr_t ExecuteEngine::ExecuteTrxRollback(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteTrxRollback" << std::endl;
#endif
  return DB_FAILED;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteExecfile(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteExecfile" << std::endl;
#endif
  auto start_time = std::chrono::high_resolution_clock::now();
  std::string file_name = ast->child_->val_;
  std::ifstream file(file_name);
  if (!file.is_open()) {
    return DB_FAILED;
  }
  constexpr int buf_size = 1024;
  char cmd[buf_size];
  while (!file.eof()) {
    memset(cmd, 0, buf_size);
    file.getline(cmd, buf_size, ';');
    cmd[strlen(cmd)] = ';';
    while (cmd[0] == '\n') {
      memmove(cmd, cmd + 1, strlen(cmd));
    }
    if (strlen(cmd) == 1) {
      continue;
    }
    std::cout << "minisql > " << cmd << std::endl;
    YY_BUFFER_STATE bp = yy_scan_string(cmd);
    if (bp == nullptr) {
      LOG(ERROR) << "Failed to create yy buffer state." << std::endl;
      exit(1);
    }
    yy_switch_to_buffer(bp);

    // init parser module
    MinisqlParserInit();

    // parse
    yyparse();
    // parse result handle
    if (MinisqlParserGetError()) {
      // error
      printf("%s\n", MinisqlParserGetErrorMessage());
    }

    auto result = Execute(MinisqlGetParserRootNode());

    // clean memory after parse
    MinisqlParserFinish();
    yy_delete_buffer(bp);
    yylex_destroy();

    // quit condition
    ExecuteInformation(result);
    if (result == DB_QUIT) {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration<double>(end_time - start_time);
      std::cout << "Execute file \"" << file_name << "\" OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
      return DB_QUIT;
    }
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end_time - start_time);
  std::cout << "Execute file \"" << file_name << "\" OK (" << std::fixed << std::setprecision(2) << duration.count() << " sec)" << std::endl;
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t ExecuteEngine::ExecuteQuit(pSyntaxNode ast, ExecuteContext *context) {
#ifdef ENABLE_EXECUTE_DEBUG
  LOG(INFO) << "ExecuteQuit" << std::endl;
#endif
  return DB_QUIT;
}
