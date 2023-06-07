#include "record/schema.h"

/**
*  Student Implement
*/
uint32_t Schema::SerializeTo(char *buf) const {
  // replace with your code here
  uint32_t ofs=4;
  uint32_t col_cnt=columns_.size();
  memcpy(buf,&col_cnt,4);

  for(Column *t_col:columns_){
    ofs+=t_col->SerializeTo(buf+ofs);
  }
  return ofs;
}

/**
*  Student Implement
*/
uint32_t Schema::GetSerializedSize() const {
  // replace with your code here
  uint32_t ofs=4;
  for(Column *t_col:columns_){
    ofs+=t_col->GetSerializedSize();
  }
  return ofs;
}

/**
*  Student Implement
*/
uint32_t Schema::DeserializeFrom(char *buf, Schema *&schema) {
  // replace with your code here
  uint32_t ofs=4;
  uint32_t col_cnt=0;
  memcpy(&col_cnt,buf,4);
  std::vector<Column *>cols(col_cnt);
  for(uint32_t i=0;i<col_cnt;i++){
    ofs+=Column::DeserializeFrom(buf+ofs,cols[i]);
  }
  schema=new Schema(cols, true);
  return ofs;
}