#pragma once

#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_API shotFileSqlInfo
    : public std::enable_shared_from_this<shotFileSqlInfo>,
      public fileSqlInfo {

 public:
  shotFileSqlInfo();
  void select(const int64_t &ID_);

  void insert() override;
  void updateSQL() override;

  static shotInfoPtrList getAll(const episodesPtr &EP_);
  static shotInfoPtrList getAll(const shotPtr &sh_);
  static shotInfoPtrList getAll(const shotClassPtr &class_ptr);
  static shotInfoPtrList getAll(const shotTypePtr &type_ptr);

  dpath generatePath(const dstring &programFolder) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes) override;
  dpath generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) override;
  dstring generateFileName(const dstring &suffixes) override;
  dstring generateFileName(const dstring &suffixes, const dstring &prefix) override;
  //外键查询
  episodesPtr getEpisdes();
  void setEpisdes(const episodesPtr &eps_);

  shotPtr getShot();
  void setShot(const shotPtr &shot_);

  shotClassPtr getShotclass();

  shotTypePtr getShotType();
  void setShotType(const shotTypePtr &fileType_);

  shotTypePtr findFileType(const std::string &type_str);
 private:
  void setShotClass(const shotClassPtr &value);
  //循环获得查询结果
  template<typename T>
  void batchSetAttr(T &row);

 private:
  qint64 p_eps_id;
  qint64 p_shot_id;
  qint64 p_shCla_id;
  qint64 p_shTy_id;

  episodesPtr p_ptr_eps;
  shotPtr p_ptr_shot;
  shotClassPtr p_ptr_shcla;
  shotTypePtr p_ptr_shTy;
};

CORE_NAMESPACE_E

