/*
 * @Author: your name
 * @Date: 2020-11-26 10:16:48
 * @LastEditTime: 2020-11-26 15:21:04
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\synData.h
 */
#pragma once

#include <core_global.h>
#include <src/coresqldata.h>
CORE_NAMESPACE_S

class CORE_API synData : public coresqldata {
 public:
  synData();
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;
  static synDataPtr getAll(const episodesPtr &episodes);
  void setEpisodes(const episodesPtr &episodes_ptr);
  episodesPtr getEpisodes();

  synPath_structPtr push_back(const assClassPtr &ass_class_ptr);

 private:
  std::string toString();
  void setSynPath(const std::string& json_str);
 private:
  episodesPtr p_episodes_;
  assClassPtrList p_class_list_;
  synPathListPtr p_path;
};
CORE_NAMESPACE_E