#include "storage/table_heap.h"

/**
*  Student Implement
*/
bool TableHeap::InsertTuple(Row &row, Transaction *txn) {
  uint32_t row_lenth=row.GetSerializedSize(schema_);
  if(row_lenth>TablePage::SIZE_MAX_ROW){
    return false;
  }

  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(first_page_id_));
  if(page==nullptr) return false;
  buffer_pool_manager_->UnpinPage(first_page_id_,false);
  page_id_t t_page_id=first_page_id_;
  page_id_t p_page_id=first_page_id_;
  while(!page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
    p_page_id=t_page_id;
    t_page_id=page->GetNextPageId();
    if(t_page_id==INVALID_PAGE_ID){
      page=reinterpret_cast<TablePage *>(buffer_pool_manager_->NewPage(t_page_id));
      if(page==nullptr) return false;
      buffer_pool_manager_->UnpinPage(t_page_id,true);
      page->Init(t_page_id,p_page_id,log_manager_,txn);
      page->SetNextPageId(INVALID_PAGE_ID);
    }else{
      page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(t_page_id));
      buffer_pool_manager_->UnpinPage(t_page_id,false);
    }
    
  }
  return true;
  // return false;
}

bool TableHeap::MarkDelete(const RowId &rid, Transaction *txn) {
  // Find the page which contains the tuple.
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  // If the page could not be found, then abort the transaction.
  if (page == nullptr) {
    return false;
  }
  // Otherwise, mark the tuple as deleted.
  page->WLatch();
  page->MarkDelete(rid, txn, lock_manager_, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
  return true;
}

/**
*  Student Implement
*/
bool TableHeap::UpdateTuple(const Row &row, const RowId &rid, Transaction *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if(page==nullptr) return false;
  Row* old_row;
  if (page->GetTuple(old_row,schema_,txn,lock_manager_))
    page->UpdateTuple(row,old_row,schema_,txn,lock_manager_,log_manager_);
  else return false;
  
  return true;
}


void TableHeap::ApplyDelete(const RowId &rid, Transaction *txn) {
  // Step1: Find the page which contains the tuple.
  // Step2: Delete the tuple from the page.
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if(page==nullptr) return ;
  page->ApplyDelete(rid,txn,log_manager_);
  return;
}

void TableHeap::RollbackDelete(const RowId &rid, Transaction *txn) {
  // Find the page which contains the tuple.
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  assert(page != nullptr);
  // Rollback to delete.
  page->WLatch();
  page->RollbackDelete(rid, txn, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
}

/**
*  Student Implement
*/
bool TableHeap::GetTuple(Row *row, Transaction *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(row->GetRowId().GetPageId()));
  assert(page != nullptr);
  return page->GetTuple(row,schema_,txn,lock_manager_);
  
}
/**
*  Student Implement
*/
void TableHeap::DeleteTable(page_id_t page_id) {
  if (page_id != INVALID_PAGE_ID) {
    auto temp_table_page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(page_id));  // 删除table_heap
    if (temp_table_page->GetNextPageId() != INVALID_PAGE_ID)
      DeleteTable(temp_table_page->GetNextPageId());
    buffer_pool_manager_->UnpinPage(page_id, false);
    buffer_pool_manager_->DeletePage(page_id);
  } else {
    DeleteTable(first_page_id_);
  }
}

/**
*  Student Implement
*/
TableIterator TableHeap::Begin(Transaction *txn) {
  RowId row_id;
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(first_page_id_));
  while(page->GetPageId()!=INVALID_PAGE_ID){
    if(page->GetFirstTupleRid(&row_id)){
      break;
    }
    page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(page->GetNextPageId()));
  }
  Row ret_row(row_id);
  TableIterator ret(&ret_row,this);
  return (const TableIterator)ret;
}

/**
*  Student Implement
*/
TableIterator TableHeap::End() {
  Row ret(INVALID_ROWID);
  return (const TableIterator) TableIterator(&ret,this);
}
