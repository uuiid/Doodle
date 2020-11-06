#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT episodes : public coresqldata {

 public:
  explicit episodes();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static episodesPtrList getAll();

  int64_t getEpisdes() const;
  dstring getEpisdes_str() const;
  void setEpisdes(const qint64 &value);

 private:
  int64_t p_int_episodes;

};

CORE_NAMESPACE_E
