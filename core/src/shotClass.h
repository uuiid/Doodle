#pragma once

#include "core_global.h"

#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT shotClass : public coresqldata {
 public:
  shotClass();
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
  };

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static shotClassPtrList getAll(const episodesPtr &episodes_ptr);
  static shotClassPtrList getAll(const shotPtr &shot_ptr);

  [[nodiscard]] dstring getClass_str() const;
  [[nodiscard]] e_fileclass getClass() const;
  void setclass(const e_fileclass &value);
  void setclass(const dstring &value);

  episodesPtr getEpisodes();
  void setEpisodes(const episodesPtr &value);

  shotPtr getShot();
  void setShot(const shotPtr &value);

 private:
  template<typename T>
  void batchSetAttr(T &row);
 private:
  e_fileclass p_fileclass;

  episodesPtr p_ptr_eps;
  shotPtr p_ptr_shot;

  qint64 p_shot_id;
  qint64 p_eps_id;
};

CORE_NAMESPACE_E
