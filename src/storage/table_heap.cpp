#include "storage/table_heap.h"

/**
*  Student Implement
*/
bool TableHeap::InsertTuple(Row &row, Transaction *txn) {
  uint32_t row_lenth=row.GetSerializedSize(schema_);
  if(row_lenth>TablePage::SIZE_MAX_ROW){
    return false;
  }
  page_id_t t_page_id=first_page_id_;
  page_id_t p_page_id=first_page_id_;
  ASSERT(first_page_id_!=INVALID_PAGE_ID,"first_page_id_ is invalid");
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(first_page_id_));
  if(page==nullptr) throw std::runtime_error("no space");
//  buffer_pool_manager_->UnpinPage(first_page_id_,false);

  while(!page->InsertTuple(row,schema_,txn,lock_manager_,log_manager_)){
    p_page_id=t_page_id;
    t_page_id=page->GetNextPageId();
    if(t_page_id==INVALID_PAGE_ID){
      auto new_page=reinterpret_cast<TablePage *>(buffer_pool_manager_->NewPage(t_page_id));
      if(new_page==nullptr) throw std::runtime_error("no space");
      new_page->Init(t_page_id,p_page_id,log_manager_,txn);
      new_page->SetNextPageId(INVALID_PAGE_ID);
      page->SetNextPageId(t_page_id);
      ASSERT(page->GetPageId() != page->GetNextPageId(), "Cycle!");
      buffer_pool_manager_->UnpinPage(page->GetPageId(), true);
      page = new_page;
      // buffer_pool_manager_->UnpinPage(t_page_id,true);
    }else{
      page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(t_page_id));
      // buffer_pool_manager_->UnpinPage(t_page_id,false);
    }
  }
  buffer_pool_manager_->UnpinPage(page->GetPageId(),true);
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
  // LOG(INFO) << "TableHeap::UpdateTuple";
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if(page==nullptr) return false;
  Row* old_row = new Row(rid);
  if (page->GetTuple(old_row,schema_,txn,lock_manager_)){
    page->UpdateTuple(row,old_row,schema_,txn,lock_manager_,log_manager_);
    buffer_pool_manager_->UnpinPage(rid.GetPageId(),true);
    delete old_row;
    return true;
  }
  else{
    buffer_pool_manager_->UnpinPage(rid.GetPageId(),false);
    delete old_row;
    return false;
  }
}


void TableHeap::ApplyDelete(const RowId &rid, Transaction *txn) {
  // Step1: Find the page which contains the tuple.
  // Step2: Delete the tuple from the page.
  auto page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if(page==nullptr) return ;
  page->ApplyDelete(rid,txn,log_manager_);
  buffer_pool_manager_->UnpinPage(rid.GetPageId(),true);
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
  // std::cout << "GetTuple" << std::endl;
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(row->GetRowId().GetPageId()));
  assert(page != nullptr);
  bool ret=page->GetTuple(row,schema_,txn,lock_manager_);
  ASSERT(ret, "Tuple not found!");
  buffer_pool_manager_->UnpinPage(row->GetRowId().GetPageId(),false);
  return ret;
  
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
  page_id_t pageId=first_page_id_;
  TablePage *page = nullptr;
  bool found= false;
  while(pageId!=INVALID_PAGE_ID){
    page=reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(pageId));
    found = page->GetFirstTupleRid(&row_id);
    page_id_t next_page_id=page->GetNextPageId();
    buffer_pool_manager_->UnpinPage(pageId, false);
    if(found){
      break;
    }
    
    pageId=next_page_id;
  }
  if(found){
    Row ret_row(row_id);
    if(row_id.GetPageId()!=INVALID_PAGE_ID){
      this->GetTuple(&ret_row,nullptr);
    }
    // std::cout << "Begin " << ret_row.GetFieldCount() << std::endl;
    // std::cout << schema_->GetColumnCount() << std::endl;
//    auto itr=new TableIterator(new Row(ret_row), this);
    return TableIterator(new Row(ret_row), this);
  }else{
    return this->End();
  }

}

/**
*  Student Implement
*/
TableIterator TableHeap::End() {
  return TableIterator(new Row(INVALID_ROWID), this);
}
