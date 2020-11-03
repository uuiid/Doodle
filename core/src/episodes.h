#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT episodes : public coresqldata {

 public:
  episodes();
  void select(const qint64 &ID_);

  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  static episodesPtrList getAll();

  qint64 getEpisdes() const;
  QString getEpisdes_str() const;
  void setEpisdes(const qint64 &value);

 private:
  qint64 p_int_episodes;

};

CORE_NAMESPACE_E
Q_DECLARE_METATYPE(doCore::episodesPtr)