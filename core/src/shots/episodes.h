/*
 * @Author: your name
 * @Date: 2020-09-15 13:40:38
 * @LastEditTime: 2020-12-14 13:32:21
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\shots\episodes.h
 */
#pragma once

#include "core_global.h"
#include "src/core/coresqldata.h"

CORE_NAMESPACE_S

class CORE_API episodes : public coresqldata,
                          public std::enable_shared_from_this<episodes> {
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
  static episodesPtr find(int64_t episodes);

 private:
  int64_t p_int_episodes;
  int64_t p_prj;

  RTTR_ENABLE(coresqldata)
};

CORE_NAMESPACE_E
