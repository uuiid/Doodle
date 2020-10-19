#pragma once

#include "core_global.h"

#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT fileClass : public coresqldata {
 Q_GADGET
 public:
  fileClass();
  void select(const qint64 &ID_);

  enum class e_fileclass {
    _ = 0,
    Executive = 1,
    Light = 2,
    VFX = 3,
    modle = 4,
    rig = 5,
    Anm = 6,
    direct = 7,
    paint = 8,
    character = 9,
    effects = 10,
    scene = 11,
    prop = 12,
    scane = 13,
    props = 14,
  };

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static fileClassPtrList getAll();
  static fileClassPtrList getAll(const episodesPtr &EP_);
  static fileClassPtrList getAll(const shotPtr &SH_);

  QString getFileclass_str() const;
  e_fileclass getFileclass() const;
  void setFileclass(const e_fileclass &value);
  void setFileclass(const QString &value);

  episodesPtr getEpisodes();
  void setEpisodes(const episodesPtrW &value);

  shotPtr getShot();
  void setShot(const shotPtrW &value);

 private:
  static fileClassPtrList batchQuerySelect(sqlQuertPtr &query);
 private:
  e_fileclass p_fileclass;

  episodesPtrW p_ptrW_eps;
  shotPtrW p_ptrW_shot;

  qint64 __shot__;
  qint64 __eps__;
};

CORE_DNAMESPACE_E

Q_DECLARE_METATYPE(doCore::fileClassPtr)