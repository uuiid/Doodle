/*
 * @Author: your name
 * @Date: 2020-11-26 10:16:48
 * @LastEditTime: 2020-12-14 13:43:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\synData.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/core/CoreData.h>
DOODLE_NAMESPACE_S

class CORE_API synData : public CoreData {
  RTTR_ENABLE(CoreData)
 public:
  synData();

  static synDataPtr getAll(const episodesPtr &episodes);
  void setEpisodes(const episodesPtr &episodes_ptr);
  episodesPtr getEpisodes();

  dpathPtr push_back(const assClassPtr &ass_class_ptr);
  synPathListPtr getSynDir(bool abspath = true);

 private:
  std::string toString();
  void setSynPath(const std::string &json_str);

 private:
  episodesPtr p_episodes_;
  assClassPtrList p_class_list_;
  synPathListPtr p_path;
};
DOODLE_NAMESPACE_E