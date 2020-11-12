//
// Created by teXiao on 2020/11/12.
//

#include <core_global.h>

CORE_NAMESPACE_S
class coreDataManager {
 public:
  //单例使用
  static coreDataManager& get();
  coreDataManager &operator=(const coreDataManager &s) = delete;
  coreDataManager(const coreDataManager &s) = delete;
 public:
  const episodesPtrList &GetEpisodeL() const;
  void SetEpisodeL(const episodesPtrList &eps);
  void SetEpisodeL(const episodesPtr &eps);

  const episodesPtr &GetEpisodesPtr() const;
  void SetEpisodesPtr(const episodesPtr &ep);

  const shotPtrList &GetShotL() const;
  void SetShotL(const shotPtrList &shots);
  void SetShotL(const shotPtr  &shots);

  const shotPtr &GetShotPtr() const;
  void SetShotPtr(const shotPtr &shot);

  const shotClassPtrList &GetShotClassL() const;
  void SetShotClassL(const shotClassPtrList &sh_c_las);
  void SetShotClassL(const shotClassPtr  &sh_c_las);

  const shotClassPtr &GetShotClassPtr() const;
  void SetShotClassPtr(const shotClassPtr &sh_cl);

  const shotTypePtrList &GetShotTypeL() const;
  void SetShotTypeL(const shotTypePtrList &sh_types);
  void SetShotTypeL(const shotTypePtr &sh_types);

  const shotTypePtr &GetShotTypePtr() const;
  void SetShotTypePtr(const shotTypePtr &sh_type);

  const shotInfoPtrList &GetShotInfoL() const;
  void SetShotInfoL(const shotInfoPtrList &sh_infos);
  void SetShotInfoL(const shotInfoPtr &sh_infos);

  const shotInfoPtr &GetShotInfoPtr() const;
  void SetShotInfoPtr(const shotInfoPtr &sh_info);

  const assDepPtrList &GetAssDepL() const;
  void SetAssDepL(const assDepPtrList &ass_deps);
  void SetAssDepL(const assDepPtr &ass_deps);

  const assDepPtr &GetAssDepPtr() const;
  void SetAssDepPtr(const assDepPtr &ass_dep);

  const assClassPtrList &GetAssClassL() const;
  void SetAssClassL(const assClassPtrList &ass_c_lss);
  void SetAssClassL(const assClassPtr &ass_c_lss);

  const assClassPtr &GetAssClassPtr() const;
  void SetAssClassPtr(const assClassPtr &ass_cl);

  const assTypePtrList &GetAssTypeL() const;
  void SetAssTypeL(const assTypePtrList &ass_types);
  void SetAssTypeL(const assTypePtr &ass_types);

  const assTypePtr &GetAssTypePtr() const;
  void SetAssTypePtr(const assTypePtr &ass_type);

  const assInfoPtrList &GetAssInfoL() const;
  void SetAssInfoL(const assInfoPtrList &ass_infos);
  void SetAssInfoL(const assInfoPtr &ass_infos);

  const assInfoPtr &GetAssInfoPtr() const;
  void SetAssInfoPtr(const assInfoPtr &ass_info);

 private:
  coreDataManager();
 private:
  episodesPtrList p_eps_;
  episodesPtr p_ep_;
  shotPtrList p_shots_;
  shotPtr p_shot_;
  shotClassPtrList p_shCLas_;
  shotClassPtr p_shCL_;
  shotTypePtrList p_shTypes_;
  shotTypePtr p_shType_;
  shotInfoPtrList p_shInfos_;
  shotInfoPtr p_shInfo_;

  assDepPtrList p_assDeps_;
  assDepPtr p_assDep_;
  assClassPtrList p_assCLss_;
  assClassPtr p_assCL_;
  assTypePtrList p_assTypes_;
  assTypePtr p_assType_;
  assInfoPtrList p_assInfos_;
  assInfoPtr p_assInfo_;
};
CORE_NAMESPACE_E
