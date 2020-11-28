/*
 * @Author: your name
 * @Date: 2020-11-26 10:17:07
 * @LastEditTime: 2020-11-27 10:04:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\synData.cpp
 */
#include <Logger.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <src/assets/assClass.h>
#include <src/coreOrm/synfile_sqlOrm.h>
#include <src/core/coreset.h>
#include <src/core/coresql.h>
#include <src/shots/episodes.h>
#include <src/sysData/synData.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
CORE_NAMESPACE_S
synData::synData() : coresqldata(), p_path(), p_episodes_() {}
void synData::insert() {
  if (isInsert()) return;

  doodle::Synfile tab{};
  auto db = coreSql::getCoreSql().getConnection();

  idP = db->insert(sqlpp::insert_into(tab).set(
      tab.path = toString(), tab.episodesId = p_episodes_->getIdP()));
  if (idP == 0) {
    DOODLE_LOG_INFO << "not install";
    throw std::runtime_error("not install");
  }
}
synDataPtr synData::getAll(const episodesPtr &episodes) {
  if (!episodes) return nullptr;

  doodle::Synfile tab{};
  auto db = coreSql::getCoreSql().getConnection();
  auto ptr = std::make_shared<synData>();

  for (const auto &row :
       db->run(sqlpp::select(sqlpp::all_of(tab))
                   .from(tab)
                   .where(tab.episodesId == episodes->getIdP()))) {
    ptr->idP = row.id;
    ptr->setSynPath(row.path);
    ptr->setEpisodes(episodes::find(row.id));
  }

  return ptr;
}
void synData::updateSQL() {
  if (isNULL()) return;

  doodle::Synfile tab{};
  auto db = coreSql::getCoreSql().getConnection();

  db->update(
      sqlpp::update(tab).set(tab.path = toString()).where(tab.id == idP));
}
void synData::deleteSQL() {
  if (isNULL()) return;
  doodle::Synfile tab{};
  auto db = coreSql::getCoreSql().getConnection();

  db->remove(sqlpp::remove_from(tab).where(tab.id == idP));
}
dpathPtr synData::push_back(const assClassPtr &ass_class_ptr) {
  auto data = synPath_struct{};
  data.server =
      p_episodes_->getEpisdes_str() + "/" + ass_class_ptr->getAssClass();
  data.local =
      p_episodes_->getEpisdes_str() + "/" + ass_class_ptr->getAssClass();
  auto iter =
      std::find_if(p_path.begin(), p_path.end(),
                   [=](synPath_struct &d) { return d.server == data.server; });
  if (iter == p_path.end()) {
    p_path.push_back(data);
  }
  return std::make_shared<dpath>(data.server);
}
episodesPtr synData::getEpisodes() { return p_episodes_; }
void synData::setEpisodes(const episodesPtr &episodes_ptr) {
  p_episodes_ = episodes_ptr;
}
std::string synData::toString() {
  nlohmann::json root{};
  for (const auto &path : p_path) {
    root.push_back({{"Left", path.local.generic_string()},
                    {"Right", path.server.generic_string()}});
  }
  return root.dump();
}
void synData::setSynPath(const std::string &json_str) {
  try {
    auto root = nlohmann::json::parse(json_str);
    for (auto &item : root) {
      synPath_struct data{};
      data.server = item.value("Left", "");
      data.local = item.value("Right", "");
      p_path.push_back(data);
    }
  } catch (nlohmann::json::parse_error &error) {
    DOODLE_LOG_INFO << "not json" << error.what();
  }
}
CORE_NAMESPACE_E
