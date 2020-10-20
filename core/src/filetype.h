#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT fileType : public coresqldata {
  Q_GADGET
 public:
  fileType();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  //根据fileclass外键查询filetype
  static fileTypePtrList getAll(const fileClassPtr &fc_);
  //根据assType 外键查询
  static fileTypePtrList getAll(const assTypePtr &AT_);
  //根据eps外键查询
  static fileTypePtrList getAll(const episodesPtr &EP_);
  //根据shot外键查询
  static fileTypePtrList getAll(const shotPtr &SH_);

  void setFileType(const QString &value);
  //获得本身的字符串属性
  QString getFileType() const;

  //设置和连接外键 fileclass
  void setFileClass(const fileClassPtrW &fileclass_);
  //获得外键连接的实体对象 fileclass
  fileClassPtr getFileClass();

  //设置和连接外键 assType
  void setAssType(const assTypePtrW &assType_);
  //获得assType实体引用
  assTypePtr getAssType();

  //设置和连接集数外键约束
  void setEpisodes(const episodesPtrW &value);
  //获得集数实体外键引用
  episodesPtr getEpisdes();

  void setShot(const shotPtrW &shot_);
  shotPtr getShot();

 private:
  //生成批量生成fileclass
  static fileTypePtrList batchQuerySelect(sqlQuertPtr &query);

 private:
  //自身属性
  QString p_Str_Type;
  //指针属性
  fileClassPtrW p_fileClass;
  assTypePtrW p_assType;
  episodesPtrW p_episdes;
  shotPtrW p_shot;
  //指针id属性
  qint64 __file_class__;
  qint64 __ass_class__;
  qint64 __episodes__;
  qint64 __shot__;
};

CORE_NAMESPACE_E

Q_DECLARE_METATYPE(doCore::fileTypePtr)