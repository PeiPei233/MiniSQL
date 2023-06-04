#include "storage/table_iterator.h"

#include "common/macros.h"
#include "storage/table_heap.h"

/**
* The whole file Student Implement
*/
TableIterator::TableIterator(Row* row_,TableHeap *table_heap_):row_(row_),table_heap_(table_heap_) {}

TableIterator::TableIterator(){}

TableIterator::TableIterator(const TableIterator &other) {
  this->row_=other.row_;
  this->table_heap_=other.table_heap_;
}

TableIterator::~TableIterator() {
  delete row_;
  // delete table_heap_;
}

bool TableIterator::operator==(const TableIterator &itr) const {
  if(!(this->row_->GetRowId()==itr.row_->GetRowId())) return false;

  return true;
}

bool TableIterator::operator!=(const TableIterator &itr) const {
  return !((*this)==itr);
}

const Row &TableIterator::operator*() {
  return *row_;
}

Row *TableIterator::operator->() {
  return row_;
}

TableIterator &TableIterator::operator=(const TableIterator &itr) noexcept {
  this->row_=itr.row_;
  this->table_heap_=itr.table_heap_;
  return *this;
}

// ++iter
TableIterator &TableIterator::operator++() {
  const RowId old_id=row_->GetRowId();
  RowId new_id;
  page_id_t new_page_id=old_id.GetPageId();
  while(new_page_id!=INVALID_PAGE_ID){
    auto page=reinterpret_cast<TablePage *>(table_heap_->buffer_pool_manager_->FetchPage(old_id.GetPageId()));
    if(page->GetNextTupleRid(old_id,&new_id)){
      Row new_row(new_id);
      this->row_=&new_row;
      if(table_heap_->End()!=*this){
        table_heap_->GetTuple(row_,nullptr);
      }
      table_heap_->buffer_pool_manager_->UnpinPage(new_page_id,false);
      
      break;
    }
    page_id_t next_page_id=page->GetNextPageId();
    table_heap_->buffer_pool_manager_->UnpinPage(new_page_id,false);
    new_page_id=next_page_id;
  }
  if (new_page_id == INVALID_PAGE_ID) {
    this->row_->SetRowId(INVALID_ROWID);
  }
  return *this;
}

// iter++
TableIterator TableIterator::operator++(int) {
  TableIterator ret_itr(*this);
  ++(*this);
  return (const TableIterator)ret_itr;
}
