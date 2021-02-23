#include "episodes.h"

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/ShotModifySQLDate.h>

#include <loggerlib/Logger.h>
#include <boost/format.hpp>

//反射使用
#include <rttr/registration>

DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<episodes>(DOCORE_RTTE_CLASS(episodes))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(episodes);
episodes::episodes()
    : CoreData(),
      std::enable_shared_from_this<episodes>(),
      p_int_episodes(-1),
      p_prj(coreSet::getSet().projectName().first),
      p_shot_modify() {
  p_instance.insert(this);
}

episodes::~episodes() {
  p_instance.erase(this);
}

void episodes::insert() {
  if (idP > 0) return;
  if (p_prj < 0) return;

  auto db = coreSql::getCoreSql().getConnection();

  // auto insert = sqlpp::insert_into(table)
  //                   .set(table.episodes  = p_int_episodes,
  //                        table.projectId = p_prj);

  // idP = db->insert(insert);
  if (idP == 0) {
    DOODLE_LOG_WARN("无法插入集数" << p_int_episodes);
    throw std::runtime_error("not install eps");
  }
}
void episodes::updateSQL() {
  if (isInsert()) {
  }
}

void episodes::deleteSQL() {
}

episodesPtrList episodes::getAll() {
}

void episodes::setEpisdes(const int64_t &value) {
  p_int_episodes = value;
}

std::shared_ptr<ShotModifySQLDate> episodes::ShotModifySqlDate() const {
  return p_shot_modify;
}

episodesPtr episodes::find_by_id(int64_t id_) {
  for (auto &&eps_ptr : p_instance) {
    if (eps_ptr->idP == id_) {
      return eps_ptr->shared_from_this();
    }
  }
  return nullptr;
}

episodesPtr episodes::find_by_eps(int64_t episodes_) {
  for (auto &&eps_ptr : p_instance) {
    if (eps_ptr->getEpisdes() == episodes_) {
      return eps_ptr->shared_from_this();
    }
  }
  return nullptr;
}

const std::unordered_set<episodes *> episodes::Instances() {
  return p_instance;
}

int64_t episodes::getEpisdes() const {
  return p_int_episodes;
}

dstring episodes::getEpisdes_str() const {
  boost::format str(DOODLE_EPFORMAT);
  str % p_int_episodes;
  return str.str();
}
QString episodes::getEpisdes_QStr() const {
  return QString::fromStdString(getEpisdes_str());
}

DOODLE_NAMESPACE_E
