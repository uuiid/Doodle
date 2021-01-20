/*
 * @Author: your name
 * @Date: 2020-11-12 14:12:09
 * @LastEditTime: 2020-12-14 17:37:44
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coreDataManager.h
 */
//
// Created by teXiao on 2020/11/12.
//
#pragma once

#include <core_global.h>

DOODLE_NAMESPACE_S
class CORE_API coreDataManager : public boost::noncopyable_::noncopyable {
 public:
  //单例使用
  static coreDataManager &get();
  coreDataManager &operator=(const coreDataManager &s) = delete;
  coreDataManager(const coreDataManager &s)            = delete;

 public:
  const episodesPtr &getEpisodesPtr() const;
  void setEpisodesPtr(const episodesPtr &ep);

  const shotPtr &getShotPtr() const;
  void setShotPtr(const shotPtr &shot);

  const shotClassPtr &getShotClassPtr() const;
  void setShotClassPtr(const shotClassPtr &sh_cl);

  const shotTypePtr &getShotTypePtr() const;
  void setShotTypePtr(const shotTypePtr &sh_type);

  const shotInfoPtr &getShotInfoPtr() const;
  void setShotInfoPtr(const shotInfoPtr &sh_info);

  const assDepPtr &getAssDepPtr() const;
  void setAssDepPtr(const assDepPtr &ass_dep);

  const assClassPtr &getAssClassPtr() const;
  void setAssClassPtr(const assClassPtr &ass_cl);

  const assTypePtr &getAssTypePtr() const;
  void setAssTypePtr(const assTypePtr &ass_type);

  const assInfoPtr &getAssInfoPtr() const;
  void setAssInfoPtr(const assInfoPtr &ass_info);

 private:
  coreDataManager();

 private:
  episodesPtr p_ep_;
  shotPtr p_shot_;
  shotClassPtr p_shCL_;
  shotTypePtr p_shType_;
  shotInfoPtr p_shInfo_;

  assDepPtr p_assDep_;
  assClassPtr p_assCL_;
  assTypePtr p_assType_;
  assInfoPtr p_assInfo_;
};
DOODLE_NAMESPACE_E
