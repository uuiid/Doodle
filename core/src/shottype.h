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
  //根据fileclass外键查询filetype
  static shotTypePtrList getAll(const shotClassPtr &fc_);
  //根据assType 外键查询
  static shotTypePtrList getAll(const assClassPtr &AT_);
  //根据eps外键查询
  static shotTypePtrList getAll(const episodesPtr &EP_);
  //根据shot外键查询
  static shotTypePtrList getAll(const shotPtr &SH_);

  void setFileType(const QString &value);
  //获得本身的字符串属性
  QString getFileType() const;

  //设置和连接外键 fileclass
  void setFileClass(const fileClassPtrW &fileclass_);
  //获得外键连接的实体对象 fileclass
  shotClassPtr getFileClass();

  //设置和连接外键 assClass
  void setAssType(const assTypePtrW &assType_);
  //获得assType实体引用
  assClassPtr getAssType();

  //设置和连接集数外键约束
  void setEpisodes(const episodesPtrW &value);
  //获得集数实体外键引用
  episodesPtr getEpisdes();

  void setShot(const shotPtrW &shot_);
  shotPtr getShot();

 private:

 private:
  //自身属性
  dstring p_Str_Type;
  //指针属性
  shotClassPtr p_fileClass;
  assClassPtr p_assType;
  episodesPtr p_episdes;
  shotPtr p_shot;
  //指针id属性
  int64_t p_shotClass_id;
  int64_t p_eps_id;
  int64_t p_shot_id;
};

CORE_NAMESPACE_E
