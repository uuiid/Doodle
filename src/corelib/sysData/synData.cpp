/*
 * @Author: your name
 * @Date: 2020-11-26 10:17:07
 * @LastEditTime: 2020-12-14 13:43:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\synData.cpp
 */
#include <loggerlib/Logger.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <corelib/assets/assClass.h>
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>
#include <corelib/sysData/synData.h>

#include <nlohmann/json.hpp>
#include <stdexcept>

DOODLE_NAMESPACE_S

synData::synData() : CoreData(), p_path(), p_episodes_() {}

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
  return std::make_shared<fileSys::path>(data.server);
}

synPathListPtr synData::getSynDir(bool abspath) {
  //   auto &set = coreSet::getSet();
  //   synPathListPtr parts{};
  //   if (abspath) {
  //     for (auto &item : p_path) {
  //       auto k_path  = synPath_struct();
  //       k_path.local = set.getSynPathLocale() / set.projectName().second /
  //                      item.local / DOODLE_CONTENT / "shot";
  //       k_path.server = set.getAssRoot() / set.getDepartment() / item.server /
  //                       DOODLE_CONTENT / "shot";
  //       parts.push_back(k_path);
  //     }
  //     return parts;
  //   } else {
  //     for (auto &item : p_path) {
  //       auto k_path   = synPath_struct();
  //       k_path.local  = item.local / DOODLE_CONTENT / "shot";
  //       k_path.server = item.server / DOODLE_CONTENT / "shot";
  //       parts.push_back(k_path);
  //     }
  //     return parts;
  //   }

  //   return p_path;
  return {};
}

episodesPtr synData::getEpisodes() { return p_episodes_; }

void synData::setEpisodes(const episodesPtr &episodes_ptr) {
  // p_episodes_ = episodes_ptr;
}
std::string synData::toString() {
  // nlohmann::json root{};
  // for (const auto &path : p_path) {
  //   root.push_back({{"Left", path.local.generic_string()},
  //                   {"Right", path.server.generic_string()}});
  // }
  // return root.dump();
  return {};
}
void synData::setSynPath(const std::string &json_str) {
  // try {
  //   auto root = nlohmann::json::parse(json_str);
  //   for (auto &item : root) {
  //     synPath_struct data{};
  //     data.server = item.value("Left", "");
  //     data.local  = item.value("Right", "");
  //     p_path.push_back(data);
  //   }
  // } catch (nlohmann::json::parse_error &error) {
  //   DOODLE_LOG_INFO("not json" << error.what());
  // }
}
DOODLE_NAMESPACE_E
