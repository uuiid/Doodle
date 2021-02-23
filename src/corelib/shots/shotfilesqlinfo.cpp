#include "shotfilesqlinfo.h"
#include <corelib/Exception/Exception.h>
#include <loggerlib/Logger.h>
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>
#include <corelib/shots/shotClass.h>
#include <corelib/shots/shottype.h>

#include <corelib/fileDBInfo/CommentInfo.h>
#include <corelib/fileDBInfo/pathParsing.h>
#include <corelib/Exception/Exception.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <memory>

//反射使用
#include <rttr/registration>
DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<shotFileSqlInfo>(DOCORE_RTTE_CLASS(shotFileSqlInfo))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

boost::signals2::signal<void()> shotFileSqlInfo::insertChanged{};
boost::signals2::signal<void()> shotFileSqlInfo::updateChanged{};
DOODLE_INSRANCE_CPP(shotFileSqlInfo);

shotFileSqlInfo::shotFileSqlInfo()
    : fileSqlInfo(),
      std::enable_shared_from_this<shotFileSqlInfo>(),
      p_shot_id(-1),
      p_eps_id(-1),
      p_shCla_id(-1),
      p_shTy_id(-1),
      p_ptr_eps(),
      p_ptr_shot(),
      p_ptr_shTy(),
      p_ptr_shcla() {
  p_instance.insert(this);
}

shotFileSqlInfo::~shotFileSqlInfo() {
  p_instance.erase(this);
}
void shotFileSqlInfo::select(const qint64& ID_) {
}

void shotFileSqlInfo::insert() {
}

void shotFileSqlInfo::updateSQL() {
}

template <typename T>
void shotFileSqlInfo::batchSetAttr(T& row) {
  idP           = row.id;
  fileP         = row.file;
  fileSuffixesP = row.fileSuffixes;
  userP         = row.user;
  versionP      = row.version;
  fileStateP    = row.filestate;

  p_parser_info->Info(row.infor);
  p_parser_path->Path(row.FilePath_);

  if (row.shotsId._is_valid) p_shot_id = row.shotsId;

  if (row.shotClassId._is_valid) {
    p_shCla_id = row.shotClassId;
    getShotType();
  }
  if (row.shotTypeId._is_valid) p_shTy_id = row.shotTypeId;
}

shotInfoPtrList shotFileSqlInfo::getAll(const episodesPtr& EP_) {
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr& sh_) {
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr& shot_ptr,
                                        const shotTypePtr& type_ptr) {
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotClassPtr& class_ptr) {
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotTypePtr& type_ptr) {
}

fileSys::path shotFileSqlInfo::generatePath(const dstring& programFolder) {
  //第一次格式化添加根路径
  fileSys::path path = coreSet::getSet().getShotRoot();

  //第二次格式化添加集数字符串
  path = path / getEpisdes()->getEpisdes_str();

  //第三次格式化添加镜头字符串
  auto shot = getShot();
  if (shot) path = path / shot->getShotAndAb_str();

  //第四次格式化添加程序文件夹
  path = path / programFolder;

  //第五次格式化添加部门文件夹
  path = path / coreSet::getSet().getDepartment();

  //第六次格式化添加类别文件夹
  path = path / getShotType()->getType();

  return path;
}

fileSys::path shotFileSqlInfo::generatePath(const dstring& programFolder,
                                            const dstring& suffixes) {
  return generatePath(programFolder) / generateFileName(suffixes);
}

fileSys::path shotFileSqlInfo::generatePath(const dstring& programFolder,
                                            const dstring& suffixes,
                                            const dstring& prefix) {
  return generatePath(programFolder) / generateFileName(suffixes, prefix);
}

dstring shotFileSqlInfo::generateFileName(const dstring& suffixes) {
  boost::format name("shot_%s_%s_%s_%s_v%04i_%s%s");

  //第一次 格式化添加 集数
  episodesPtr ep_ = getEpisdes();
  if (ep_)
    name % ep_->getEpisdes_str();
  else
    name % "";

  //第二次格式化添加 镜头号
  shotPtr sh_ = getShot();
  if (sh_)
    name % sh_->getShotAndAb_str();
  else
    name % "";

  //第三次格式化添加 fileclass
  if (getShotclass())
    name % coreSet::getSet().getDepartment();
  else
    name % "";

  //第四次格式化添加 shotType
  shotTypePtr ft_ = getShotType();
  if (ft_)
    name % ft_->getType();
  else
    name % "";

  name % versionP;
  name % coreSet::getSet().getUser_en();
  name % suffixes;

  return name.str();
}

dstring shotFileSqlInfo::generateFileName(const dstring& suffixes,
                                          const dstring& prefix) {
  boost::format name("%s_%s");
  name % prefix;
  name % generateFileName(suffixes);
  return name.str();
}

dataInfoPtr shotFileSqlInfo::findSimilar() {
  auto it =
      std::find_if(
          p_instance.begin(), p_instance.end(),
          [=](shotFileSqlInfo* info) -> bool {
            if (info)
              return info->p_eps_id == p_eps_id &&
                     info->p_shot_id == std::max(p_shot_id, (qint64)0) &&
                     info->p_shTy_id == std::max(p_shTy_id, (qint64)0) &&
                     info->p_shCla_id == std::max(p_shCla_id, (qint64)0) &&
                     info->idP > 0;
            else
              return false;
          });

  if (it != p_instance.end()) {
    (*it)->fileP         = fileP;
    (*it)->fileStateP    = fileStateP;
    (*it)->fileSuffixesP = fileSuffixesP;
    (*it)->versionP      = versionP;
    (*it)->userP         = userP;
    (*it)->p_parser_info = p_parser_info;
    (*it)->p_parser_path = p_parser_path;

    p_parser_info->setFileSql(*it);
    p_parser_path->setFileSql(*it);

    return (*it)->shared_from_this();
  } else {
    return shared_from_this();
  }
}

episodesPtr shotFileSqlInfo::getEpisdes() {
  if (p_ptr_eps) {
    return p_ptr_eps;
  } else if (p_eps_id >= 0) {
    auto epi = episodes::Instances();

    auto eps_iter =
        std::find_if(epi.begin(), epi.end(),
                     [=](episodes* ptr) -> bool { return ptr->getIdP() == p_eps_id; });
    if (eps_iter != epi.end()) {
      p_ptr_eps = (*eps_iter)->shared_from_this();

    } else {
      throw nullptr_error(std::string("shotfileInfo id : ")
                              .append(std::to_string(getIdP()))
                              .append("  ")
                              .append(getUser()));
    }

    this->setEpisdes(p_ptr_eps);
    return p_ptr_eps;
  }
  return nullptr;
}

void shotFileSqlInfo::setEpisdes(const episodesPtr& eps_) {
  if (!eps_) return;

  p_ptr_eps = eps_;
  p_eps_id  = eps_->getIdP();
}

shotPtr shotFileSqlInfo::getShot() {
  if (p_ptr_shot != nullptr) {
    return p_ptr_shot;
  } else if (p_shot_id >= 0) {
    shotPtr p_ = std::make_shared<shot>();
    p_->select(p_shot_id);
    p_ptr_shot = p_;
    return p_;
  }
  return nullptr;
}

void shotFileSqlInfo::setShot(const shotPtr& shot_) {
  if (!shot_) return;
  p_ptr_shot = shot_;
  p_shot_id  = shot_->getIdP();

  setEpisdes(shot_->getEpisodes());
}

shotClassPtr shotFileSqlInfo::getShotclass() {
  if (p_ptr_shcla)
    return p_ptr_shcla;
  else if (p_shCla_id >= 0) {
    auto& k_instance = shotClass::Instances();
    auto it          = std::find_if(k_instance.begin(), k_instance.end(),
                           [=](shotClass* item) -> bool {
                             return item->getIdP() == p_shCla_id;
                           });
    if (it != k_instance.end()) {
      p_ptr_shcla = (*it)->shared_from_this();
    }
    return p_ptr_shcla;
  }
  return nullptr;
}

void shotFileSqlInfo::setShotClass() {
  try {
    auto value = shotClass::getCurrentClass();
    if (!value) return;
    p_shCla_id  = value->getIdP();
    p_ptr_shcla = value;
  } catch (const std::runtime_error& err) {
    DOODLE_LOG_WARN(err.what());
  }
}

shotTypePtr shotFileSqlInfo::getShotType() {
  if (p_ptr_shTy)
    return p_ptr_shTy;
  else if (p_shTy_id > 0) {
    for (const auto& item : shotType::Instances()) {
      if (item->getIdP() == p_shTy_id) {
        p_ptr_shTy = item->shared_from_this();
        break;
      }
    }
    return p_ptr_shTy;
  }
  return nullptr;
}
void shotFileSqlInfo::setShotType(const shotTypePtr& fileType_) {
  if (!fileType_) return;
  p_shTy_id  = fileType_->getIdP();
  p_ptr_shTy = fileType_;

  versionP = getVersionMax() + 1;
  setShotClass();
}
bool shotFileSqlInfo::sort(const shotInfoPtr& t1, const shotInfoPtr& t2) {
  auto t1_class = t1->getShotclass();
  auto t2_class = t2->getShotclass();
  auto t1_type  = t1->getShotType();
  auto t2_type  = t2->getShotType();
  if (t1_class && t2_class && t1_type && t2_type) {
    return t1_class->getClass_str() < t2_class->getClass_str() &&
           t1_type->getType() < t2_type->getType();
  } else {
    return false;
  }
}
int shotFileSqlInfo::getVersionMax() {
  for (const auto& info_l : p_instance) {
    if (info_l != nullptr)
      try {
        if ((getShotType() == info_l->getShotType()) &&
            (info_l->getShotclass() == shotClass::getCurrentClass()) &&
            info_l->idP > 0)
          return info_l->versionP;
      } catch (const std::runtime_error& e) {
        return 0;
        std::cerr << e.what() << '\n';
      }
  }
  return 0;
}
const std::unordered_set<shotFileSqlInfo*> shotFileSqlInfo::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
