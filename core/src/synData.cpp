#include <src/synData.h>

#include <src/coresql.h>
#include <src/episodes.h>
#include <src/assClass.h>

#include <Logger.h>
#include <stdexcept>
#include <src/coreOrm/synfile_sqlOrm.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

CORE_NAMESPACE_S
synData::synData() {

}
void synData::insert() {
  if (isInsert()) return;

  doodle::Synfile tab{};
  auto db = coreSql::getCoreSql().getConnection();

  idP = db->insert(sqlpp::insert_into(tab).set(
      tab.path = toString(),
      tab.episodesId = p_episodes_->getIdP()
      ));
  if (idP == 0) {
    DOODLE_LOG_INFO << "not install";
    throw std::runtime_error("not install");
  }
}
void synData::updateSQL() {

}
void synData::deleteSQL() {

}
assClassPtr doCore::synData::getAssClass() {
  return assClassPtr();
}
void synData::push_back(const assClassPtr &ass_class_ptr) {
  p_class_list_.push_back(ass_class_ptr);
}
episodesPtr synData::getEpisodes() {
  return episodesPtr();
}
void synData::setEpisodes(const episodesPtr &episodes_ptr) {

}
std::string synData::toString() {
  return std::string();
}
CORE_NAMESPACE_E
