/*
 * @Author: your name
 * @Date: 2020-09-15 14:23:34
 * @LastEditTime: 2020-12-14 16:29:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assets\assClass.h
 */
#pragma once

#include "core_global.h"

#include "src/core/coresqldata.h"

CORE_NAMESPACE_S

class CORE_API assClass : public coresqldata,
                          public std::enable_shared_from_this<assClass> {
 public:
  assClass();
  ~assClass();
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assClassPtrList getAll(const assDepPtr &ass_dep_ptr);
  [[nodiscard]] assDepPtr getAssDep() const;
  void setAssDep(const assDepPtr &value);

  std::string getAssClass() const;
  std::string getAssClass(const bool &isZNCH);
  QString getAssClassQ(bool isZNCH);

  void setAssClass(const std::string &value);
  void setAssClass(const std::string &value, const bool &isZNCH);

  static const std::unordered_set<assClass *> Instances();

 private:
  std::string name;

  qint64 p_assDep_id;
  assDepPtr p_ass_dep_ptr_;

  znchNamePtr p_ptr_znch;
  DOODLE_INSRANCE(assClass);
  RTTR_ENABLE(coresqldata);
};
inline QString assClass::getAssClassQ(bool isZNCH) {
  return QString::fromStdString(getAssClass(isZNCH));
}
CORE_NAMESPACE_E
