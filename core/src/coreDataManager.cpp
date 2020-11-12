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
const episodesPtrList &coreDataManager::GetEpisodeL() const {
  return p_eps_;
}
void coreDataManager::SetEpisodeL(const episodesPtrList &eps) {
  coreDataManager::p_eps_ = eps;
}
const episodesPtr &coreDataManager::GetEpisodesPtr() const {
  return p_ep_;
}
void coreDataManager::SetEpisodesPtr(const episodesPtr &ep) {
  coreDataManager::p_ep_ = ep;
}
const shotPtrList &coreDataManager::GetShotL() const {
  return p_shots_;
}
void coreDataManager::SetShotL(const shotPtrList &shots) {
  coreDataManager::p_shots_ = shots;
}
const shotPtr &coreDataManager::GetShotPtr() const {
  return p_shot_;
}
void coreDataManager::SetShotPtr(const shotPtr &shot) {
  coreDataManager::p_shot_ = shot;
}
const shotClassPtrList &coreDataManager::GetShotClassL() const {
  return p_shCLas_;
}
void coreDataManager::SetShotClassL(const shotClassPtrList &sh_c_las) {
  p_shCLas_ = sh_c_las;
}
const shotClassPtr &coreDataManager::GetShotClassPtr() const {
  return p_shCL_;
}
void coreDataManager::SetShotClassPtr(const shotClassPtr &sh_cl) {
  p_shCL_ = sh_cl;
}
const shotTypePtrList &coreDataManager::GetShotTypeL() const {
  return p_shTypes_;
}
void coreDataManager::SetShotTypeL(const shotTypePtrList &sh_types) {
  p_shTypes_ = sh_types;
}
const shotTypePtr &coreDataManager::GetShotTypePtr() const {
  return p_shType_;
}
void coreDataManager::SetShotTypePtr(const shotTypePtr &sh_type) {
  p_shType_ = sh_type;
}
const shotInfoPtrList &coreDataManager::GetShotInfoL() const {
  return p_shInfos_;
}
void coreDataManager::SetShotInfoL(const shotInfoPtrList &sh_infos) {
  p_shInfos_ = sh_infos;
}
const shotInfoPtr &coreDataManager::GetShotInfoPtr() const {
  return p_shInfo_;
}
void coreDataManager::SetShotInfoPtr(const shotInfoPtr &sh_info) {
  p_shInfo_ = sh_info;
}
const assDepPtrList &coreDataManager::GetAssDepL() const {
  return p_assDeps_;
}
void coreDataManager::SetAssDepL(const assDepPtrList &ass_deps) {
  p_assDeps_ = ass_deps;
}
const assDepPtr &coreDataManager::GetAssDepPtr() const {
  return p_assDep_;
}
void coreDataManager::SetAssDepPtr(const assDepPtr &ass_dep) {
  p_assDep_ = ass_dep;
}
const assClassPtrList &coreDataManager::GetAssClassL() const {
  return p_assCLss_;
}
void coreDataManager::SetAssClassL(const assClassPtrList &ass_c_lss) {
  p_assCLss_ = ass_c_lss;
}
const assClassPtr &coreDataManager::GetAssClassPtr() const {
  return p_assCL_;
}
void coreDataManager::SetAssClassPtr(const assClassPtr &ass_cl) {
  p_assCL_ = ass_cl;
}
const assTypePtrList &coreDataManager::GetAssTypeL() const {
  return p_assTypes_;
}
void coreDataManager::SetAssTypeL(const assTypePtrList &ass_types) {
  p_assTypes_ = ass_types;
}
const assTypePtr &coreDataManager::GetAssTypePtr() const {
  return p_assType_;
}
void coreDataManager::SetAssTypePtr(const assTypePtr &ass_type) {
  p_assType_ = ass_type;
}
const assInfoPtrList &coreDataManager::GetAssInfoL() const {
  return p_assInfos_;
}
void coreDataManager::SetAssInfoL(const assInfoPtrList &ass_infos) {
  p_assInfos_ = ass_infos;
}
const assInfoPtr &coreDataManager::GetAssInfoPtr() const {
  return p_assInfo_;
}
void coreDataManager::SetAssInfoPtr(const assInfoPtr &ass_info) {
  p_assInfo_ = ass_info;
}
void coreDataManager::SetEpisodeL(const episodesPtr &eps) {
  p_eps_.push_back(eps);
}
void coreDataManager::SetShotL(const shotPtr &shots) {
  p_shots_.push_back(shots);
}
void coreDataManager::SetShotClassL(const shotClassPtr &sh_c_las) {
  p_shCLas_.push_back(sh_c_las);
}
void coreDataManager::SetShotTypeL(const shotTypePtr &sh_types) {
  p_shTypes_.push_back(sh_types);
}
void coreDataManager::SetShotInfoL(const shotInfoPtr &sh_infos) {
  p_shInfos_.push_back(sh_infos);
}
void coreDataManager::SetAssDepL(const assDepPtr &ass_deps) {
  p_assDeps_.push_back(ass_deps);
}
void coreDataManager::SetAssClassL(const assClassPtr &ass_c_lss) {
  p_assCLss_.push_back(ass_c_lss);
}
void coreDataManager::SetAssTypeL(const assTypePtr &ass_types) {
  p_assTypes_.push_back(ass_types);
}
void coreDataManager::SetAssInfoL(const assInfoPtr &ass_infos) {
  p_assInfos_.push_back(ass_infos);
}

CORE_NAMESPACE_E
