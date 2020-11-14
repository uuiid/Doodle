#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_API episodes : public coresqldata,
                          public std::enable_shared_from_this<episodes>{

 public:
  explicit episodes();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static episodesPtrList getAll();

  int64_t getEpisdes() const;
  dstring getEpisdes_str() const;
  QString getEpisdes_QStr() const;
  void setEpisdes(const int64_t &value);

 private:
  int64_t p_int_episodes;
  int64_t p_prj;

};

CORE_NAMESPACE_E
