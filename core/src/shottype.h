#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT shotType : public coresqldata {
 public:
  shotType();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  //根据eps外键查询
  static shotTypePtrList getAll(const episodesPtr &EP_);
  //根据shot外键查询
  static shotTypePtrList getAll(const shotPtr &SH_);
  //根据fileclass外键查询filetype
  static shotTypePtrList getAll(const shotClassPtr &fc_);
  //根据assType 外键查询
  static shotTypePtrList getAll(const shotType &AT_);

  void setType(const dstring &value);
  //获得本身的字符串属性
  dstring getType() const;

  //设置和连接外键 shotclass
  void setFileClass(const shotClassPtr &fileclass_);
  //获得外键连接的实体对象 shotclass
  shotClassPtr getFileClass();


  //设置和连接集数外键约束
  void setEpisodes(const episodesPtr &value);
  //获得集数实体外键引用
  episodesPtr getEpisdes();

  void setShot(const shotPtr &shot_);
  shotPtr getShot();

 private:
  template<typename T>
  void batchSetAttr(T &row);
 private:
  //自身属性
  dstring p_Str_Type;

  //指针属性
  shotPtr p_shot;
  episodesPtr p_episdes;
  shotClassPtr p_class_ptr_;

  //指针id属性
  int64_t p_shotClass_id;
  int64_t p_eps_id;
  int64_t p_shot_id;
};

CORE_NAMESPACE_E
