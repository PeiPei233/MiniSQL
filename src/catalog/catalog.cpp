#include "catalog/catalog.h"
#include "page/index_roots_page.h"
void CatalogMeta::SerializeTo(char *buf) const {
    ASSERT(GetSerializedSize() <= PAGE_SIZE, "Failed to serialize catalog metadata to disk.");
    MACH_WRITE_UINT32(buf, CATALOG_METADATA_MAGIC_NUM);
    buf += 4;
    MACH_WRITE_UINT32(buf, table_meta_pages_.size());
    buf += 4;
    MACH_WRITE_UINT32(buf, index_meta_pages_.size());
    buf += 4;
    for (auto iter : table_meta_pages_) {
        MACH_WRITE_TO(table_id_t, buf, iter.first);
        buf += 4;
        MACH_WRITE_TO(page_id_t, buf, iter.second);
        buf += 4;
    }
    for (auto iter : index_meta_pages_) {
        MACH_WRITE_TO(index_id_t, buf, iter.first);
        buf += 4;
        MACH_WRITE_TO(page_id_t, buf, iter.second);
        buf += 4;
    }
}

CatalogMeta *CatalogMeta::DeserializeFrom(char *buf) {
    // check valid
    uint32_t magic_num = MACH_READ_UINT32(buf);
    buf += 4;
    ASSERT(magic_num == CATALOG_METADATA_MAGIC_NUM, "Failed to deserialize catalog metadata from disk.");
    // get table and index nums
    uint32_t table_nums = MACH_READ_UINT32(buf);
    buf += 4;
    uint32_t index_nums = MACH_READ_UINT32(buf);
    buf += 4;
    // create metadata and read value
    CatalogMeta *meta = new CatalogMeta();
    for (uint32_t i = 0; i < table_nums; i++) {
        auto table_id = MACH_READ_FROM(table_id_t, buf);
        buf += 4;
        auto table_heap_page_id = MACH_READ_FROM(page_id_t, buf);
        buf += 4;
        meta->table_meta_pages_.emplace(table_id, table_heap_page_id);
    }
    for (uint32_t i = 0; i < index_nums; i++) {
        auto index_id = MACH_READ_FROM(index_id_t, buf);
        buf += 4;
        auto index_page_id = MACH_READ_FROM(page_id_t, buf);
        buf += 4;
        meta->index_meta_pages_.emplace(index_id, index_page_id);
    }
    return meta;
}

/**
 * Student Implement
 */
uint32_t CatalogMeta::GetSerializedSize() const {
  uint32_t ret=12+table_meta_pages_.size()*8+index_meta_pages_.size()*8;
  return ret;
}

CatalogMeta::CatalogMeta() {
//  table_meta_pages_.clear();
//  index_meta_pages_.clear();
}

/**
 * Student Implement
 */
CatalogManager::CatalogManager(BufferPoolManager *buffer_pool_manager, LockManager *lock_manager,
                               LogManager *log_manager, bool init)
    : buffer_pool_manager_(buffer_pool_manager), lock_manager_(lock_manager), log_manager_(log_manager) {
  if(init){
    catalog_meta_=CatalogMeta::NewInstance();
    return ;
  }else{
    Page *cata_page=buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    catalog_meta_ = CatalogMeta::DeserializeFrom(cata_page->GetData());
    std::map<table_id_t, page_id_t> table_meta_page=*(catalog_meta_->GetTableMetaPages());
    std::map<index_id_t ,page_id_t >index_meta_page=*(catalog_meta_->GetIndexMetaPages());
    for(auto &itr:table_meta_page){
      LoadTable(itr.first,itr.second);
    }
    for(auto &itr:index_meta_page){
      LoadIndex(itr.first,itr.second);
    }
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    FlushCatalogMetaPage();
  }
}

CatalogManager::~CatalogManager() {
 /** After you finish the code for the CatalogManager section,
 *  you can uncomment the commented code. Otherwise it will affect b+tree test**/
  FlushCatalogMetaPage();
  delete catalog_meta_;/////////////////
  for (auto iter : tables_) {
    delete iter.second;
  }
  for (auto iter : indexes_) {
    delete iter.second;
  }

}

/**
* Student Implement
*/
dberr_t CatalogManager::CreateTable(const string &table_name, TableSchema *schema,
                                    Transaction *txn, TableInfo *&table_info) {
  // ASSERT(false, "Not Implemented yet");
  auto tmp=table_names_.find(table_name);
  if(tmp!=table_names_.end()){
    return DB_TABLE_ALREADY_EXIST;
  }else{
    page_id_t pageId;
    Page *root_page=buffer_pool_manager_->NewPage(pageId);
    table_id_t new_id=catalog_meta_->GetNextTableId();
    table_names_.emplace(table_name,new_id);
    TableSchema *myschema=Schema::DeepCopySchema(schema);
    TableMetadata *table_meta=TableMetadata::Create(new_id,table_name,pageId,myschema);
    table_meta->SerializeTo(root_page->GetData());
    TableHeap *tableHeap=TableHeap::Create(buffer_pool_manager_,
                                             INVALID_PAGE_ID,//pageID
                                             myschema,
                                             nullptr,
                                             nullptr);
    table_info=TableInfo::Create();
    table_info->Init(table_meta,tableHeap);
//    table_names_.emplace(table_name,new_id);
    tables_.emplace(new_id,table_info);
    index_names_.emplace(table_name, std::unordered_map<std::string, index_id_t>());
    catalog_meta_->AddTableMetaPage(new_id,pageId);

    Page *cata_page=buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    catalog_meta_->SerializeTo(cata_page->GetData());
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    buffer_pool_manager_->UnpinPage(pageId, true);

    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::GetTable(const string &table_name, TableInfo *&table_info) {
  auto table=table_names_.find(table_name);
  if(table==table_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    table_info=tables_.find(table->second)->second;
    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::GetTables(vector<TableInfo *> &tables) const {
  for(auto itr:tables_){
    tables.push_back(itr.second);
  }
  return DB_FAILED;
}

/**
 * Student Implement
 */
dberr_t CatalogManager::CreateIndex(const std::string &table_name, const string &index_name,
                                    const std::vector<std::string> &index_keys, Transaction *txn,
                                    IndexInfo *&index_info, const string &index_type) {
  // ASSERT(false, "Not Implemented yet");
  auto table=index_names_.find(table_name);
  if(table==index_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    auto index=table->second.find(index_name);
    if(index!=table->second.end()){
      return DB_INDEX_ALREADY_EXIST;
    }else{
      index_id_t new_id=catalog_meta_->GetNextIndexId();
      table_id_t tableId=table_names_.find(table_name)->second;
      TableInfo *tableInfo=tables_.find(tableId)->second;
      std::vector<uint32_t> key_map;
      std::vector<Column*> cols=tableInfo->GetSchema()->GetColumns();

      key_map.clear();
      for(auto &itr:index_keys){
        uint32_t i=0;
        for(;i<cols.size();i++){
          if(cols[i]->GetName()==itr){
            key_map.push_back(i);
            break;
          }
        }
        if(i==cols.size()){
          return DB_COLUMN_NAME_NOT_EXIST;
        }
      }
      IndexMetadata *index_meta=IndexMetadata::Create(new_id,index_name,tableId,key_map);
      page_id_t pageId;
      Page *root_page=buffer_pool_manager_->NewPage(pageId);
      index_info=IndexInfo::Create();
      index_info->Init(index_meta,tableInfo,buffer_pool_manager_);
      indexes_.emplace(new_id,index_info);
      table->second.emplace(index_name,new_id);
      index_meta->SerializeTo(root_page->GetData());
      catalog_meta_->AddIndexMetaPage(new_id,pageId);
      Page *cata_page=buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
      catalog_meta_->SerializeTo(cata_page->GetData());
      buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID,true);
      buffer_pool_manager_->UnpinPage(pageId,true);
      return DB_SUCCESS;
    }
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::GetIndex(const std::string &table_name, const std::string &index_name,
                                 IndexInfo *&index_info) const {
  // ASSERT(false, "Not Implemented yet");
  auto table=index_names_.find(table_name);
  if(table==index_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    auto index=table->second.find(index_name);
    if(index==table->second.end()){
      return DB_INDEX_NOT_FOUND;
    }else{
      index_info=indexes_.find(index->second)->second;


      return DB_SUCCESS;
    }
  }

}

/**
 * Student Implement
 */
dberr_t CatalogManager::GetTableIndexes(const std::string &table_name, std::vector<IndexInfo *> &indexes) const {
  auto tmp=index_names_.find(table_name);
  if(tmp==index_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    for(auto &itr:tmp->second){
      indexes.push_back(indexes_.find(itr.second)->second);
    }
    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::DropTable(const string &table_name) {
  auto tmp=table_names_.find(table_name);
  if(tmp==table_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    table_names_.erase(tmp);
    table_id_t table_id=tmp->second;
    TableInfo *tableInfo=tables_.find(table_id)->second;
    page_id_t root_id=tableInfo->GetRootPageId();
    buffer_pool_manager_->DeletePage(root_id);
    tables_.erase(tables_.find(table_id));

    catalog_meta_->DeleteTableMetaPage(buffer_pool_manager_,table_id);
    Page *cata_page=buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    catalog_meta_->SerializeTo(cata_page->GetData());
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    return DB_SUCCESS;
  }

}

/**
 * Student Implement
 */
dberr_t CatalogManager::DropIndex(const string &table_name, const string &index_name) {
  auto tmp=index_names_.find(table_name);
  if(tmp==index_names_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    auto id=tmp->second;
    auto tmp2=id.find(index_name);
    if(tmp2==id.end()){
      return DB_INDEX_NOT_FOUND;
    }
    index_names_.erase(tmp);
    indexes_.erase(indexes_.find(tmp2->second));
    catalog_meta_->DeleteIndexMetaPage(buffer_pool_manager_,tmp2->second);

    Page *cata_page=buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    catalog_meta_->SerializeTo(cata_page->GetData());
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::FlushCatalogMetaPage() const {
  // ASSERT(false, "Not Implemented yet");
  buffer_pool_manager_->FlushPage(CATALOG_META_PAGE_ID);
  return DB_SUCCESS;
}

/**
 * Student Implement
 */
dberr_t CatalogManager::LoadTable(const table_id_t table_id, const page_id_t page_id) {
  Page *table_page=buffer_pool_manager_->FetchPage(page_id);
  if(table_page== nullptr)
    return DB_FAILED;
  else {
    char *buf = table_page->GetData();
    TableMetadata *table_meta = nullptr;
    TableMetadata::DeserializeFrom(buf, table_meta);
    table_names_.emplace(table_meta->GetTableName(), table_id);
    TableHeap *heap = TableHeap::Create(buffer_pool_manager_,
                                        table_meta->GetFirstPageId(),
                                        table_meta->GetSchema(),
                                        nullptr, nullptr);
    TableInfo *info = TableInfo::Create();
    info->Init(table_meta, heap);
    tables_.emplace(table_id, info);
    buffer_pool_manager_->UnpinPage(page_id, true);
    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::LoadIndex(const index_id_t index_id, const page_id_t page_id) {
  // ASSERT(false, "Not Implemented yet");
  Page *index_page=buffer_pool_manager_->FetchPage(page_id);
  if(index_page==nullptr)
    return DB_FAILED;
  else{
    char *buf=index_page->GetData();
    IndexMetadata *index_meta=nullptr;
    IndexMetadata::DeserializeFrom(buf,index_meta);
    table_id_t tableId=index_meta->GetTableId();
    std::string table_name=tables_.find(tableId)->second->GetTableName();
    std::unordered_map<std::string,index_id_t > index_map;
    index_map.clear();
    index_map.emplace(index_meta->GetIndexName(),index_id);
    index_names_.emplace(table_name,index_map);
    IndexInfo *id_info=IndexInfo::Create();
    TableInfo *table_info=tables_.find(tableId)->second;
    id_info->Init(index_meta,table_info,buffer_pool_manager_);
    indexes_.emplace(index_id,id_info);
    buffer_pool_manager_->UnpinPage(page_id, true);
    return DB_SUCCESS;
  }
}

/**
 * Student Implement
 */
dberr_t CatalogManager::GetTable(const table_id_t table_id, TableInfo *&table_info) {
  auto tmp=tables_.find(table_id);
  if(tmp==tables_.end()){
    return DB_TABLE_NOT_EXIST;
  }else{
    table_info=tmp->second;
    return DB_SUCCESS;
  }
}