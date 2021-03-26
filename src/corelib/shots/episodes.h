/*
 * @Author: your name
 * @Date: 2020-09-15 13:40:38
 * @LastEditTime: 2020-12-14 15:25:36
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\shots\episodes.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/core/CoreData.h>

DOODLE_NAMESPACE_S

class CORE_API episodes : public CoreData,
                          public std::enable_shared_from_this<episodes> {

 public:
  explicit episodes();
  ~episodes();

  static episodesPtrList getAll();

  bool setInfo(const std::string &value) override;

  int64_t getEpisdes() const;
  dstring getEpisdes_str() const;
  void setEpisdes(const int64_t &value);
  std::shared_ptr<ShotModifySQLDate> ShotModifySqlDate() const;

  static episodesPtr find_by_id(int64_t id_);
  static episodesPtr find_by_eps(int64_t episodes_);
  static const std::unordered_set<episodes *> Instances();
  //声明友元类

 private:
  int64_t p_int_episodes;
  projectPtr p_prj;

  std::shared_ptr<ShotModifySQLDate> p_shot_modify;
  DOODLE_INSRANCE(episodes);
};

DOODLE_NAMESPACE_E
