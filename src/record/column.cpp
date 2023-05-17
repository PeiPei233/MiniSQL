#include "record/column.h"

#include "glog/logging.h"

Column::Column(std::string column_name, TypeId type, uint32_t index, bool nullable, bool unique)
    : name_(std::move(column_name)), type_(type), table_ind_(index), nullable_(nullable), unique_(unique) {
  ASSERT(type != TypeId::kTypeChar, "Wrong constructor for CHAR type.");
  switch (type) {
    case TypeId::kTypeInt:
      len_ = sizeof(int32_t);
      break;
    case TypeId::kTypeFloat:
      len_ = sizeof(float_t);
      break;
    default:
      ASSERT(false, "Unsupported column type.");
  }
}

Column::Column(std::string column_name, TypeId type, uint32_t length, uint32_t index, bool nullable, bool unique)
    : name_(std::move(column_name)),
      type_(type),
      len_(length),
      table_ind_(index),
      nullable_(nullable),
      unique_(unique) {
  ASSERT(type == TypeId::kTypeChar, "Wrong constructor for non-VARCHAR type.");
}

Column::Column(const Column *other)
    : name_(other->name_),
      type_(other->type_),
      len_(other->len_),
      table_ind_(other->table_ind_),
      nullable_(other->nullable_),
      unique_(other->unique_) {}

/**
*  Student Implement
*/
uint32_t Column::SerializeTo(char *buf) const {
  // replace with your code here
  uint32_t ofs=0;
  uint32_t name_lenth=name_.length();
  memcpy(buf,&name_lenth,4);
  ofs+=4;

  memcpy(buf+ofs,name_.c_str(),name_lenth);
  ofs+=name_lenth;

  memcpy(buf+ofs,&type_,4);
  ofs+=4;

  memcpy(buf+ofs,&len_,4);
  ofs+=4; 

  memcpy(buf+ofs,&table_ind_,4);
  ofs+=4; 

  memcpy(buf+ofs,&nullable_,sizeof(bool));
  ofs+=sizeof(bool);
  
  memcpy(buf+ofs,&unique_,sizeof(bool));
  ofs+=sizeof(bool);
  return ofs;
}

/**
*  Student Implement
*/
uint32_t Column::GetSerializedSize() const {
  // replace with your code here
  uint32_t ofs=0;
  ofs+=name_.length()+16+2*sizeof(bool);
  return ofs;
}

/**
*  Student Implement
*/
uint32_t Column::DeserializeFrom(char *buf, Column *&column) {
  // replace with your code here
  uint32_t ofs=0;
  uint32_t name_lenth;
  memcpy(&name_lenth,buf,4);
  ofs+=4;
  char *tmp_name=new char[name_lenth];
  memcpy(tmp_name,buf+ofs,name_lenth);
  ofs+=name_lenth;
  column->name_=tmp_name;
  delete tmp_name;

  uint32_t type_id;
  memcpy(&type_id,buf+ofs,4);
  column->type_=TypeId(type_id);
  ofs+=4;

  memcpy(&(column->len_),buf+ofs,4);
  ofs+=4;

  memcpy(&(column->table_ind_),buf+ofs,4);
  ofs+=4;

  memcpy(&(column->nullable_),buf+ofs,sizeof(bool));
  ofs+=sizeof(bool);

  memcpy(&(column->unique_),buf+ofs,sizeof(bool));
  ofs+=sizeof(bool);

  return ofs;
}
