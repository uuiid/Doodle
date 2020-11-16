#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_API shotType : public coresqldata,
                          public std::enable_shared_from_this<shotType> {
 public:
  shotType();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  //根据fileclass外键查询filetype
  static shotTypePtrList getAll();


  void setType(const dstring &value);
  void setType(const QString &value);
  //获得本身的字符串属性
  dstring getType() const;
  QString getTypeQ() const;
  //获得外键连接的实体对象 shotclass
  shotClassPtr getFileClass();

  //根据字符串寻找tyep
  static shotTypePtr findShotType(std::string &type_name);
  static shotTypePtr findShotType(std::string &type_nmae, bool autoInstall);
 private:
  template<typename T>
  void batchSetAttr(T &row);
  
  //设置和连接外键 shotclass
  void setShotClass(const shotClassPtr &fileclass_);
 private:
  //自身属性
  dstring p_Str_Type;

  //指针属性
  shotClassPtr p_class_ptr_;

  //指针id属性
  int64_t p_shotClass_id;

};

inline void shotType::setType(const QString &value) {
  setType(value.toStdString());
}
inline QString shotType::getTypeQ() const {
  return QString::fromStdString(getType());
}
CORE_NAMESPACE_E
