#include "record/row.h"

/**
*  Student Implement
*/
uint32_t Row::SerializeTo(char *buf, Schema *schema) const {
   ASSERT(schema != nullptr, "Invalid schema before serialize.");
   ASSERT(schema->GetColumnCount() == fields_.size(), "Fields size do not match schema's column size.");
  // replace with your code here
  uint32_t ofs=sizeof(uint32_t);
  uint32_t cnt=this->GetFieldCount();
  uint32_t lenth=(cnt+31)/32;
  // auto_ptr<>
  // printf("cnt=%d from row::ser",cnt);
  uint32_t *null_map=(uint32_t*)malloc(4*lenth);
  memset(null_map,0,4*lenth);
  memcpy(buf,&cnt,sizeof(uint32_t));
  ofs+=4*lenth;
  for(int i=0;i<cnt;i++){
    if(fields_[i]->IsNull()){
      null_map[i/32]|=1<<(i%32);
      // ofs+=fields_[i]->SerializeTo(buf+ofs);
    }else{
      ofs+=fields_[i]->SerializeTo(buf+ofs);
    }
  }
  memcpy(buf+4,null_map,4*lenth);
  return ofs;
  // return 0;
}

/**
*  Student Implement
*/
uint32_t Row::DeserializeFrom(char *buf, Schema *schema) {
  // replace with your code here

  fields_.clear();
  
  uint32_t ofs=sizeof(uint32_t);
  uint32_t cnt=0;
  memcpy(&cnt,buf,sizeof(uint32_t));
  // printf("cnt=%d from row::deser",cnt);
  uint32_t lenth=(cnt+31)/32;
  uint32_t *null_map=(uint32_t*)malloc(4*lenth);
  memcpy(null_map,buf+4,4*lenth);
  ofs+=4*lenth;
  fields_.resize(cnt);
  for(int i=0;i<lenth;i++){
    uint32_t tmp=0x0001;
    int j;
    for(j=0;j<32;j++){
      uint32_t cur=i*32+j;
      if(cur==cnt) break;
      Field *t_field=new Field(schema->GetColumn(cur)->GetType());
      if((null_map[i]&tmp)!=0){
        // Field t_field(0)
        // fields_[cur] = new Field(*t_field);
        ofs+=t_field->DeserializeFrom(buf+ofs,schema->GetColumn(cur)->GetType(),&fields_[cur],true);
      }else{
        ofs+=t_field->DeserializeFrom(buf+ofs,schema->GetColumn(cur)->GetType(),&fields_[cur],false);
      }
      tmp=tmp<<1;
      delete t_field;
    }
    if(j!=32) break;
  }
  return ofs;

}

/**
*  Student Implement
*/
uint32_t Row::GetSerializedSize(Schema *schema) const {
  // ASSERT(schema != nullptr, "Invalid schema before serialize.");
  // ASSERT(schema->GetColumnCount() == fields_.size(), "Fields size do not match schema's column size.");
  // replace with your code here
  uint32_t ofs=sizeof(uint32_t);
  uint32_t cnt=schema->GetColumnCount();
  uint32_t lenth=(cnt+31)/32;
  ofs+=4*lenth;
  for(int i=0;i<cnt;i++){
    if(!fields_[i]->IsNull()){
      ofs+=fields_[i]->GetSerializedSize();
    }
  }
  return ofs;
}

void Row::GetKeyFromRow(const Schema *schema, const Schema *key_schema, Row &key_row) {
  auto columns = key_schema->GetColumns();
  std::vector<Field> fields;
  uint32_t idx;
  for (auto column : columns) {
    schema->GetColumnIndex(column->GetName(), idx);
    fields.emplace_back(*this->GetField(idx));
  }
  key_row = Row(fields);
  key_row.SetRowId(this->GetRowId());
}
