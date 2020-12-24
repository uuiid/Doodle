/*
 * @Author: your name
 * @Date: 2020-10-19 13:26:31
 * @LastEditTime: 2020-12-14 15:59:21
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\shotClass.h
 */
#pragma once

#include <core_global.h>
#include <src/core/coresqldata.h>

#include <boost/signals2.hpp>
CORE_NAMESPACE_S

class CORE_API shotClass : public coresqldata,
                           public std::enable_shared_from_this<shotClass> {
 public:
  shotClass();
  ~shotClass();
  void select(const qint64 &ID_);

  enum class e_fileclass {
    _         = 0,
    Executive = 1,
    Light     = 2,
    VFX       = 3,
    modle     = 4,
    rig       = 5,
    Anm       = 6,
    direct    = 7,
    paint     = 8,
  };

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static shotClassPtrList getAll();
  static shotClassPtr getCurrentClass();

  [[nodiscard]] dstring getClass_str() const;
  [[nodiscard]] QString getClass_Qstr() const;
  [[nodiscard]] e_fileclass getClass() const;
  void setclass(const e_fileclass &value);
  void setclass(const dstring &value);
  void setclass(const QString &value);

  static const std::unordered_set<shotClass *> Instances();

  DOODLE_DISABLE_COPY(shotClass);

  static boost::signals2::signal<void()> insertChanged;

 private:
  template <typename T>
  void batchSetAttr(T &row);

 private:
  e_fileclass p_fileclass;
  DOODLE_INSRANCE(shotClass);
  RTTR_ENABLE(coresqldata)
};
inline QString shotClass::getClass_Qstr() const {
  return QString::fromStdString(getClass_str());
}
inline void shotClass::setclass(const QString &value) {
  setclass(value.toStdString());
}
CORE_NAMESPACE_E
