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
  this->row_= new Row(*(itr.row_));
  this->table_heap_=itr.table_heap_;
  return *this;
}

// ++iter
TableIterator &TableIterator::operator++() {
  std::cout << "TableIterator::operator++()" << std::endl;
  RowId old_id=row_->GetRowId();
  RowId new_id;
  page_id_t new_page_id=old_id.GetPageId();
  if(new_page_id!=INVALID_PAGE_ID){
    auto page=reinterpret_cast<TablePage *>(table_heap_->buffer_pool_manager_->FetchPage(new_page_id));
    if(page->GetNextTupleRid(old_id,&new_id)){
      this->row_=new Row(new_id);
      table_heap_->GetTuple(this->row_, nullptr);
      table_heap_->buffer_pool_manager_->UnpinPage(new_page_id,false);
//      break;
      return *this;
    }else{
      page_id_t next_page_id=page->GetNextPageId();

      ASSERT(page->GetPageId() != page->GetNextPageId(), "Cycle!");
      table_heap_->buffer_pool_manager_->UnpinPage(new_page_id,false);
      if(next_page_id==INVALID_PAGE_ID){
        this->row_->SetRowId(INVALID_ROWID);
        return *this;
      }
      page=reinterpret_cast<TablePage *>(table_heap_->buffer_pool_manager_->FetchPage(next_page_id));
      old_id.Set(next_page_id,-1);
      if(page->GetNextTupleRid(old_id,&new_id)){
        this->row_=new Row(new_id);
        table_heap_->GetTuple(this->row_, nullptr);
        table_heap_->buffer_pool_manager_->UnpinPage(next_page_id, false);
        return *this;
      }
//      new_page_id=next_page_id;
    }
  }
  this->row_->SetRowId(INVALID_ROWID);
  return *this;


}

// iter++
TableIterator TableIterator::operator++(int) {
  TableIterator ret_itr(*this);
  ++(*this);
  return (const TableIterator)ret_itr;
}
