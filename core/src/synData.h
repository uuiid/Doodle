/*
 * @Author: your name
 * @Date: 2020-11-26 10:16:48
 * @LastEditTime: 2020-11-26 10:19:26
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

  void setEpisodes(const episodesPtr &episodes_ptr);
  episodesPtr getEpisodes();

  void push_back(const assClassPtr &ass_class_ptr);
  assClassPtr getAssClass();

 private:
  std::string toString();
 private:
  episodesPtr p_episodes_;
  assClassPtrList p_class_list_;
  synPathListPtr p_path;
};
CORE_NAMESPACE_E