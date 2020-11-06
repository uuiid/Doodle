#pragma once

#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_EXPORT shotFileSqlInfo : public fileSqlInfo {
 Q_GADGET
 public:
  shotFileSqlInfo();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static shotInfoPtrList getAll(const episodesPtr &EP_);
  static shotInfoPtrList getAll(const shotPtr &sh_);
  static shotInfoPtrList getAll(const fileClassPtr &fc_);
  static shotInfoPtrList getAll(const fileTypePtr &ft_);

  dpath generatePath(const std::string &programFolder) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) override;
  dstring generateFileName(const dstring &suffixes) override;
  dstring generateFileName(const dstring &suffixes, const dstring &prefix) override;
  //外键查询
  episodesPtr getEpisdes();
  void setEpisdes(const episodesPtrW &eps_);

  shotPtr getShot();
  void setShot(const shotPtrW &shot_);

  fileClassPtr getFileclass();
  void setFileClass(const fileClassPtrW &value);

  fileTypePtr getFileType();
  void setFileType(const fileTypePtrW &fileType_);


  fileTypePtr findFileType(const std::string & type_str);
 private:
  //循环获得查询结果
  static shotInfoPtrList batchQuerySelect(sqlQuertPtr &query);

 private:
  qint64 __episodes__;
  qint64 __shot__;
  qint64 __file_class__;
  qint64 __file_type__;

  episodesPtrW p_ptrw_eps;
  shotPtrW p_ptrw_shot;
  fileClassPtrW p_ptrw_fileClass;
  fileTypePtrW p_ptrw_fileType;
};

CORE_NAMESPACE_E

Q_DECLARE_METATYPE(doCore::shotInfoPtr)