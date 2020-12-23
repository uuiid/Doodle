#include "shotfilesqlinfo.h"

#include <Logger.h>
#include <src/core/coreset.h>
#include <src/core/coresql.h>
#include <src/shots/episodes.h>
#include <src/shots/shot.h>
#include <src/shots/shotClass.h>
#include <src/shots/shottype.h>

#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <src/coreOrm/basefile_sqlOrm.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <memory>

//反射使用
#include <rttr/registration>
CORE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<shotFileSqlInfo>(DOCORE_RTTE_CLASS(shotFileSqlInfo))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}
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
      p_ptr_shcla() {}
shotFileSqlInfo::~shotFileSqlInfo() {
  if (isInsert() || p_instance[idP] == this)
    p_instance.erase(idP);
}
void shotFileSqlInfo::select(const qint64& ID_) {
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  for (auto&& row : db->run(
           sqlpp::select(sqlpp::all_of(tab)).from(tab).where(tab.id == ID_))) {
    batchSetAttr(row);
  }
  p_instance[idP] = this;
}

void shotFileSqlInfo::insert() {
  if (idP > 0) return;
  doodle::Basefile tab{};

  auto db      = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::dynamic_insert_into(*db, tab).dynamic_set(
      tab.file         = fileP,
      tab.fileSuffixes = fileSuffixesP,
      tab.user         = userP,
      tab.version      = versionP,
      tab.FilePath_    = filepathP,
      tab.filestate    = sqlpp::value_or_null(fileStateP),
      tab.projectId    = coreSet::getSet().projectName().first);
  if (!infoP.empty())
    install.insert_list.add(tab.infor = strList_tojson(infoP));
  if (p_shot_id > 0) install.insert_list.add(tab.shotsId = p_shot_id);
  if (p_eps_id > 0) install.insert_list.add(tab.episodesId = p_eps_id);

  DOODLE_LOG_DEBUG(shotClass::getCurrentClass()->getClass_str() << " id " << shotClass::getCurrentClass()->getIdP());

  install.insert_list.add(tab.shotClassId =
                              shotClass::getCurrentClass()->getIdP());

  if (p_shTy_id > 0) install.insert_list.add(tab.shotTypeId = p_shTy_id);
  idP = db->insert(install);
  if (idP == 0) {
    DOODLE_LOG_WARN(fileStateP.c_str());
    throw std::runtime_error("");
  }
  p_instance[idP] = this;
}

void shotFileSqlInfo::updateSQL() {
  if (idP < 0) return;
  doodle::Basefile tab{};

  auto db     = coreSql::getCoreSql().getConnection();
  auto updata = sqlpp::update(tab);
  try {
    db->update(
        updata.set(tab.infor        = strList_tojson(infoP),
                   tab.filestate    = fileStateP,
                   tab.FilePath_    = filepathP,
                   tab.file         = fileP,
                   tab.fileSuffixes = fileSuffixesP,
                   tab.version      = versionP)
            .where(tab.id == idP));
  } catch (const sqlpp::exception& err) {
    DOODLE_LOG_WARN(err.what());
  }
}

template <typename T>
void shotFileSqlInfo::batchSetAttr(T& row) {
  idP           = row.id;
  fileP         = row.file;
  fileSuffixesP = row.fileSuffixes;
  userP         = row.user;
  versionP      = row.version;
  filepathP     = row.FilePath_;
  infoP         = json_to_strList(row.infor);
  fileStateP    = row.filestate;
  if (row.shotsId._is_valid) p_shot_id = row.shotsId;
  if (row.shotClassId._is_valid) {
    p_shCla_id = row.shotClassId;
    getShotType();
  }
  if (row.shotTypeId._is_valid) p_shTy_id = row.shotTypeId;
}

shotInfoPtrList shotFileSqlInfo::getAll(const episodesPtr& EP_) {
  doodle::Basefile tab{};
  shotInfoPtrList list{};

  auto db = coreSql::getCoreSql().getConnection();
  for (auto&& row : db->run(
           sqlpp::select(sqlpp::all_of(tab))
               .from(tab)
               .where(tab.episodesId == EP_->getIdP() and tab.shotsId.is_null())
               .order_by(tab.filetime.desc()))) {
    auto Info = std::make_shared<shotFileSqlInfo>();
    Info->batchSetAttr(row);
    Info->setEpisdes(EP_);
    list.push_back(Info);
    p_instance[Info->idP] = Info.get();
  }
  return list;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr& sh_) {
  doodle::Basefile tab{};
  shotInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto&& row : db->run(sqlpp::select(sqlpp::all_of(tab))
                                .from(tab)
                                .where(tab.shotsId == sh_->getIdP())
                                .order_by(tab.filetime.desc()))) {
    auto Info = std::make_shared<shotFileSqlInfo>();
    Info->batchSetAttr(row);
    Info->setShot(sh_);
    Info->exist(true);
    list.push_back(Info);
    p_instance[Info->idP] = Info.get();
  }

  return list;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr& shot_ptr,
                                        const shotTypePtr& type_ptr) {
  doodle::Basefile tab{};
  shotInfoPtrList list;

  auto db   = coreSql::getCoreSql().getConnection();
  auto& row = db->run(sqlpp::select(sqlpp::all_of(tab))
                          .from(tab)
                          .where(tab.shotsId == shot_ptr->getIdP() and
                                 tab.shotTypeId == type_ptr->getIdP())
                          .order_by(tab.filetime.desc())
                          .limit(1u))
                  .front();

  auto Info = std::make_shared<shotFileSqlInfo>();
  if (row._is_valid) {
    Info->idP           = row.id;
    Info->fileP         = row.file.text;
    Info->fileSuffixesP = row.fileSuffixes.text;
    Info->userP         = row.user.text;
    Info->versionP      = row.version;
    Info->filepathP     = row.FilePath_.text;
    Info->infoP         = Info->json_to_strList(row.infor.text);
    Info->fileStateP    = row.filestate;
    if (row.shotsId._is_valid) Info->p_shot_id = row.shotsId;
    if (row.shotClassId._is_valid) {
      Info->p_shCla_id = row.shotClassId;
      Info->getShotType();
    }
    if (row.shotTypeId._is_valid) Info->p_shTy_id = row.shotTypeId;

    Info->exist(true);
    list.push_back(Info);
    p_instance[Info->idP] = Info.get();
  } else {
    list.push_back(nullptr);
  }
  return list;
  //  return {};
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotClassPtr& class_ptr) {
  doodle::Basefile tab{};
  shotInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto&& row : db->run(sqlpp::select(sqlpp::all_of(tab))
                                .from(tab)
                                .where(tab.shotClassId == class_ptr->getIdP())
                                .order_by(tab.filetime.desc()))) {
    auto Info = std::make_shared<shotFileSqlInfo>();
    Info->batchSetAttr(row);
    Info->exist(true);
    list.push_back(Info);
    p_instance[Info->idP] = Info.get();
  }
  return list;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotTypePtr& type_ptr) {
  doodle::Basefile tab{};
  shotInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto&& row : db->run(sqlpp::select(sqlpp::all_of(tab))
                                .from(tab)
                                .where(tab.shotTypeId == type_ptr->getIdP())
                                .order_by(tab.filetime.desc()))) {
    auto Info = std::make_shared<shotFileSqlInfo>();
    Info->batchSetAttr(row);
    Info->setShotType(type_ptr);
    Info->exist(true);
    list.push_back(Info);
    p_instance[Info->idP] = Info.get();
  }
  return list;
}

dpath shotFileSqlInfo::generatePath(const dstring& programFolder) {
  //第一次格式化添加根路径
  dpath path = coreSet::getSet().getShotRoot();

  //第二次格式化添加集数字符串
  path = path / getEpisdes()->getEpisdes_str();

  //第三次格式化添加镜头字符串
  auto shot = getShot();
  if (shot) path = path / shot->getShot_str();

  //第四次格式化添加程序文件夹
  path = path / programFolder;

  //第五次格式化添加部门文件夹
  path = path / coreSet::getSet().getDepartment();

  //第六次格式化添加类别文件夹
  path = path / getShotType()->getType();

  return path;
}

dpath shotFileSqlInfo::generatePath(const dstring& programFolder,
                                    const dstring& suffixes) {
  return generatePath(programFolder) / generateFileName(suffixes);
}

dpath shotFileSqlInfo::generatePath(const dstring& programFolder,
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
          [=](std::pair<const int64_t, shotFileSqlInfo*> info) -> bool {
            return info.second->p_eps_id == p_eps_id &&
                   info.second->p_shot_id == p_shot_id &&
                   info.second->p_shTy_id == p_shTy_id &&
                   info.second->p_shCla_id == p_shCla_id &&
                   info.second->idP > 0;
          });
  if (it != p_instance.end()) {
    it->second->fileP         = fileP;
    it->second->filepathP     = filepathP;
    it->second->fileStateP    = fileStateP;
    it->second->fileSuffixesP = fileSuffixesP;
    it->second->versionP      = versionP;
    it->second->infoP         = infoP;

    return it->second->shared_from_this();
  } else {
    return shared_from_this();
  }
}

episodesPtr shotFileSqlInfo::getEpisdes() {
  if (p_ptr_eps) {
    return p_ptr_eps;
  } else if (p_eps_id >= 0) {
    auto epi = episodes::Instances();
    auto it  = epi.find(p_eps_id);
    if (it != epi.end())
      p_ptr_eps = it->second->shared_from_this();
    else {
      episodesPtr p_ptr_eps = std::make_shared<episodes>();
      p_ptr_eps->select(p_eps_id);
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
    for (const auto& item : shotClass::Instances()) {
      if (item.second->getIdP() == p_shCla_id) {
        p_ptr_shcla = item.second->shared_from_this();
        break;
      }
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
      if (item.second->getIdP() == p_shTy_id) {
        p_ptr_shTy = item.second->shared_from_this();
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
  return (t1->getShotclass()->getClass_str() <
          t2->getShotclass()->getClass_str()) &&
         (t1->getShotType()->getType() < t2->getShotType()->getType());
}
int shotFileSqlInfo::getVersionMax() {
  for (const auto& info_l : p_instance) {
    try {
      if ((getShotType() == info_l.second->getShotType()) &&
          (info_l.second->getShotclass() == shotClass::getCurrentClass()) &&
          info_l.second->idP > 0)
        return info_l.second->versionP;
    } catch (const std::runtime_error& e) {
      return 0;
      std::cerr << e.what() << '\n';
    }
  }
  return 0;
}
const std::map<int64_t, shotFileSqlInfo*>& shotFileSqlInfo::Instances() {
  return p_instance;
}
CORE_NAMESPACE_E
