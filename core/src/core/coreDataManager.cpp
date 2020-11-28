//
// Created by teXiao on 2020/11/12.
//

#include "coreDataManager.h"

CORE_NAMESPACE_S
coreDataManager &doCore::coreDataManager::get() {
  static coreDataManager install;
  return install;
}
coreDataManager::coreDataManager()
    : p_eps_(),
      p_shot_(),
      p_shots_(),
      p_assType_(),
      p_assTypes_(),
      p_shType_(),
      p_shTypes_(),
      p_assDep_(),
      p_assInfo_(),
      p_assInfos_(),
      p_shInfo_(),
      p_shInfos_(),
      p_assCL_(),
      p_assCLss_(),
      p_assDeps_(),
      p_ep_(),
      p_shCL_(),
      p_shCLas_() {

}
const episodesPtrList &coreDataManager::getEpisodeL() const {
  return p_eps_;
}
void coreDataManager::setEpisodeL(const episodesPtrList &eps) {
  coreDataManager::p_eps_ = eps;
}
const episodesPtr &coreDataManager::getEpisodesPtr() const {
  return p_ep_;
}
void coreDataManager::setEpisodesPtr(const episodesPtr &ep) {
  coreDataManager::p_ep_ = ep;
}
const shotPtrList &coreDataManager::getShotL() const {
  return p_shots_;
}
void coreDataManager::setShotL(const shotPtrList &shots) {
  coreDataManager::p_shots_ = shots;
}
const shotPtr &coreDataManager::getShotPtr() const {
  return p_shot_;
}
void coreDataManager::setShotPtr(const shotPtr &shot) {
  coreDataManager::p_shot_ = shot;
}
const shotClassPtrList &coreDataManager::getShotClassL() const {
  return p_shCLas_;
}
void coreDataManager::setShotClassL(const shotClassPtrList &sh_c_las) {
  p_shCLas_ = sh_c_las;
}
const shotClassPtr &coreDataManager::getShotClassPtr() const {
  return p_shCL_;
}
void coreDataManager::setShotClassPtr(const shotClassPtr &sh_cl) {
  p_shCL_ = sh_cl;
}
const shotTypePtrList &coreDataManager::getShotTypeL() const {
  return p_shTypes_;
}
void coreDataManager::setShotTypeL(const shotTypePtrList &sh_types) {
  p_shTypes_ = sh_types;
}
const shotTypePtr &coreDataManager::getShotTypePtr() const {
  return p_shType_;
}
void coreDataManager::setShotTypePtr(const shotTypePtr &sh_type) {
  p_shType_ = sh_type;
}
const shotInfoPtrList &coreDataManager::getShotInfoL() const {
  return p_shInfos_;
}
void coreDataManager::setShotInfoL(const shotInfoPtrList &sh_infos) {
  p_shInfos_ = sh_infos;
}
const shotInfoPtr &coreDataManager::getShotInfoPtr() const {
  return p_shInfo_;
}
void coreDataManager::setShotInfoPtr(const shotInfoPtr &sh_info) {
  p_shInfo_ = sh_info;
}
const assDepPtrList &coreDataManager::getAssDepL() const {
  return p_assDeps_;
}
void coreDataManager::setAssDepL(const assDepPtrList &ass_deps) {
  p_assDeps_ = ass_deps;
}
const assDepPtr &coreDataManager::getAssDepPtr() const {
  return p_assDep_;
}
void coreDataManager::setAssDepPtr(const assDepPtr &ass_dep) {
  p_assDep_ = ass_dep;
}
const assClassPtrList &coreDataManager::getAssClassL() const {
  return p_assCLss_;
}
void coreDataManager::setAssClassL(const assClassPtrList &ass_c_lss) {
  p_assCLss_ = ass_c_lss;
}
const assClassPtr &coreDataManager::getAssClassPtr() const {
  return p_assCL_;
}
void coreDataManager::setAssClassPtr(const assClassPtr &ass_cl) {
  p_assCL_ = ass_cl;
}
const assTypePtrList &coreDataManager::getAssTypeL() const {
  return p_assTypes_;
}
void coreDataManager::setAssTypeL(const assTypePtrList &ass_types) {
  p_assTypes_ = ass_types;
}
const assTypePtr &coreDataManager::getAssTypePtr() const {
  return p_assType_;
}
void coreDataManager::setAssTypePtr(const assTypePtr &ass_type) {
  p_assType_ = ass_type;
}
const assInfoPtrList &coreDataManager::getAssInfoL() const {
  return p_assInfos_;
}
void coreDataManager::setAssInfoL(const assInfoPtrList &ass_infos) {
  p_assInfos_ = ass_infos;
}
const assInfoPtr &coreDataManager::getAssInfoPtr() const {
  return p_assInfo_;
}
void coreDataManager::setAssInfoPtr(const assInfoPtr &ass_info) {
  p_assInfo_ = ass_info;
}
void coreDataManager::setEpisodeL(const episodesPtr &eps) {
  p_eps_.push_back(eps);
}
void coreDataManager::setShotL(const shotPtr &shots) {
  p_shots_.push_back(shots);
}
void coreDataManager::setShotClassL(const shotClassPtr &sh_c_las) {
  p_shCLas_.push_back(sh_c_las);
}
void coreDataManager::setShotTypeL(const shotTypePtr &sh_types) {
  p_shTypes_.push_back(sh_types);
}
void coreDataManager::setShotInfoL(const shotInfoPtr &sh_infos) {
  p_shInfos_.insert(p_shInfos_.begin(), sh_infos);
}
void coreDataManager::setAssDepL(const assDepPtr &ass_deps) {
  p_assDeps_.push_back(ass_deps);
}
void coreDataManager::setAssClassL(const assClassPtr &ass_c_lss) {
  p_assCLss_.push_back(ass_c_lss);
}
void coreDataManager::setAssTypeL(const assTypePtr &ass_types) {
  p_assTypes_.push_back(ass_types);
}
void coreDataManager::setAssInfoL(const assInfoPtr &ass_infos) {
  p_assInfos_.insert(p_assInfos_.begin(), ass_infos);
}

CORE_NAMESPACE_E
