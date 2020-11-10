#pragma once

#include "core_global.h"

#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT assClass : public coresqldata {
 public:
  assClass();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static assClassPtrList getAll(const assDepPtr & ass_dep_ptr);
  [[nodiscard]] assDepPtr getAssDep() const;
  void setAssDep(const assDepPtr &value);

  std::string getAssClass() const;
  std::string getAssClass(const bool &isZNCH);

  void setAssClass(const std::string &value);
  void setAssClass(const std::string &value, const bool &isZNCH);

 private:
  std::string name;

  qint64 p_assDep_id;
  assDepPtr p_ass_dep_ptr_;

  znchNamePtr p_ptr_znch;
};

CORE_NAMESPACE_E
