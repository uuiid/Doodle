/*
 * @Author: your name
 * @Date: 2020-11-12 14:12:09
 * @LastEditTime: 2020-12-14 17:40:04
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\coreDataManager.cpp
 */
//
// Created by teXiao on 2020/11/12.
//

#include "coreDataManager.h"

DOODLE_NAMESPACE_S
coreDataManager &coreDataManager::get() {
  static coreDataManager install;
  return install;
}

const coreDataManager& coreDataManager::getConst() 
{
  return get();
}
coreDataManager::coreDataManager()
    : p_shot_(),
      p_assType_(),
      p_shType_(),
      p_assDep_(),
      p_assInfo_(),
      p_shInfo_(),
      p_assCL_(),
      p_ep_(),
      p_shCL_() {
}

const episodesPtr &coreDataManager::getEpisodesPtr() const {
  return p_ep_;
}
void coreDataManager::setEpisodesPtr(const episodesPtr &ep) {
  coreDataManager::p_ep_ = ep;
}

const shotPtr &coreDataManager::getShotPtr() const {
  return p_shot_;
}
void coreDataManager::setShotPtr(const shotPtr &shot) {
  coreDataManager::p_shot_ = shot;
}

const shotClassPtr &coreDataManager::getShotClassPtr() const {
  return p_shCL_;
}
void coreDataManager::setShotClassPtr(const shotClassPtr &sh_cl) {
  p_shCL_ = sh_cl;
}

const shotTypePtr &coreDataManager::getShotTypePtr() const {
  return p_shType_;
}
void coreDataManager::setShotTypePtr(const shotTypePtr &sh_type) {
  p_shType_ = sh_type;
}

const shotInfoPtr &coreDataManager::getShotInfoPtr() const {
  return p_shInfo_;
}
void coreDataManager::setShotInfoPtr(const shotInfoPtr &sh_info) {
  p_shInfo_ = sh_info;
}

const assDepPtr &coreDataManager::getAssDepPtr() const {
  return p_assDep_;
}
void coreDataManager::setAssDepPtr(const assDepPtr &ass_dep) {
  p_assDep_ = ass_dep;
}

const assClassPtr &coreDataManager::getAssClassPtr() const {
  return p_assCL_;
}
void coreDataManager::setAssClassPtr(const assClassPtr &ass_cl) {
  p_assCL_ = ass_cl;
}

const assTypePtr &coreDataManager::getAssTypePtr() const {
  return p_assType_;
}
void coreDataManager::setAssTypePtr(const assTypePtr &ass_type) {
  p_assType_ = ass_type;
}

const assInfoPtr &coreDataManager::getAssInfoPtr() const {
  return p_assInfo_;
}
void coreDataManager::setAssInfoPtr(const assInfoPtr &ass_info) {
  p_assInfo_ = ass_info;
}

DOODLE_NAMESPACE_E
