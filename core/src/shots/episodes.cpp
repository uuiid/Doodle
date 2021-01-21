#include "episodes.h"

#include <src/core/coreset.h>
#include <src/core/coresql.h>
#include <src/shots/ShotModifySQLDate.h>

#include <Logger.h>

#include <src/coreOrm/episodes_sqlOrm.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
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
    : coresqldata(),
      std::enable_shared_from_this<episodes>(),
      p_int_episodes(-1),
      p_prj(coreSet::getSet().projectName().first),
      p_shot_modify() {
  p_shot_modify = std::make_shared<ShotModifySQLDate>(weak_from_this());
  p_instance.insert(this);
}

episodes::~episodes() {
  p_instance.erase(this);
}

void episodes::insert() {
  if (idP > 0) return;
  if (p_prj < 0) return;
  doodle::Episodes table{};
  auto db = coreSql::getCoreSql().getConnection();

  auto insert = sqlpp::insert_into(table)
                    .set(table.episodes  = p_int_episodes,
                         table.projectId = p_prj);

  idP = db->insert(insert);
  if (idP == 0) {
    DOODLE_LOG_WARN("无法插入集数" << p_int_episodes);
    throw std::runtime_error("not install eps");
  }
}
void episodes::updateSQL() {
  if (isInsert()) {
    doodle::Episodes table{};
    auto db = coreSql::getCoreSql().getConnection();
    try {
      db->update(sqlpp::update(table)
                     .set(table.episodes = p_int_episodes)
                     .where(table.id == idP));
    } catch (const sqlpp::exception &err) {
      DOODLE_LOG_WARN(err.what());
    }
  }
}

void episodes::deleteSQL() {
  doodle::Episodes table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}

episodesPtrList episodes::getAll() {
  episodesPtrList list;

  doodle::Episodes table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row : db->run(
           sqlpp::select(sqlpp::all_of(table))
               .from(table)
               .where(table.projectId == coreSet::getSet().projectName().first)
               .order_by(table.episodes.asc()))) {
    auto eps            = std::make_shared<episodes>();
    eps->p_int_episodes = row.episodes;
    eps->idP            = row.id;
    eps->p_prj          = row.projectId;
    list.push_back(eps);
  }

  return list;
}

void episodes::setEpisdes(const int64_t &value) {
  p_int_episodes = value;
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
