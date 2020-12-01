//
// Created by teXiao on 2020/11/12.
//

#include <core_global.h>

CORE_NAMESPACE_S
class CORE_API coreDataManager {
 public:
  //单例使用
  static coreDataManager &get();
  coreDataManager &operator=(const coreDataManager &s) = delete;
  coreDataManager(const coreDataManager &s) = delete;

 public:
  const episodesPtrList &getEpisodeL() const;
  void setEpisodeL(const episodesPtrList &eps);
  void setEpisodeL(const episodesPtr &eps);

  const episodesPtr &getEpisodesPtr() const;
  void setEpisodesPtr(const episodesPtr &ep);

  const shotPtrList &getShotL() const;
  void setShotL(const shotPtrList &shots);
  void setShotL(const shotPtr &shots);

  const shotPtr &getShotPtr() const;
  void setShotPtr(const shotPtr &shot);

  const shotClassPtrList &getShotClassL() const;
  void setShotClassL(const shotClassPtrList &sh_c_las);
  void setShotClassL(const shotClassPtr &sh_c_las);

  const shotClassPtr &getShotClassPtr() const;
  void setShotClassPtr(const shotClassPtr &sh_cl);

  const shotTypePtrList &getShotTypeL() const;
  void setShotTypeL(const shotTypePtrList &sh_types);
  void setShotTypeL(const shotTypePtr &sh_types);

  const shotTypePtr &getShotTypePtr() const;
  void setShotTypePtr(const shotTypePtr &sh_type);

  const shotInfoPtrList &getShotInfoL() const;
  void setShotInfoL(const shotInfoPtrList &sh_infos);
  void setShotInfoL(const shotInfoPtr &sh_infos);

  const shotInfoPtr &getShotInfoPtr() const;
  void setShotInfoPtr(const shotInfoPtr &sh_info);

  const assDepPtrList &getAssDepL() const;
  void setAssDepL(const assDepPtrList &ass_deps);
  void setAssDepL(const assDepPtr &ass_deps);

  const assDepPtr &getAssDepPtr() const;
  void setAssDepPtr(const assDepPtr &ass_dep);

  const assClassPtrList &getAssClassL() const;
  void setAssClassL(const assClassPtrList &ass_c_lss);
  void setAssClassL(const assClassPtr &ass_c_lss);

  const assClassPtr &getAssClassPtr() const;
  void setAssClassPtr(const assClassPtr &ass_cl);

  const assTypePtrList &getAssTypeL() const;
  void setAssTypeL(const assTypePtrList &ass_types);
  void setAssTypeL(const assTypePtr &ass_types);

  const assTypePtr &getAssTypePtr() const;
  void setAssTypePtr(const assTypePtr &ass_type);

  const assInfoPtrList &getAssInfoL() const;
  void setAssInfoL(const assInfoPtrList &ass_infos);
  void setAssInfoL(const assInfoPtr &ass_infos);

  const assInfoPtr &getAssInfoPtr() const;
  void setAssInfoPtr(const assInfoPtr &ass_info);

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
