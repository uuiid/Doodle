#include "episodes.h"

#include "coresql.h"
#include "coreset.h"

#include "Logger.h"

#include "coreOrm/episodes_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <boost/format.hpp>

CORE_NAMESPACE_S

episodes::episodes() : p_int_episodes(-1),
                       p_prj(coreSet::getSet().projectName().first) {
}

void episodes::select(const qint64 &ID_) {
  doodle::Episodes table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.id == ID_)
  )) {
    p_int_episodes = row.episodes;
    idP = row.id;
  }
}

void episodes::insert() {
  if (idP > 0) return;
  if (p_prj < 0) return;
  doodle::Episodes table{};
  auto db = coreSql::getCoreSql().getConnection();

  auto insert = sqlpp::insert_into(table)
      .set(table.episodes = p_int_episodes,
           table.projectId = p_prj);


  idP = db->insert(insert);
  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入集数" << p_int_episodes;
    throw std::runtime_error("not install eps");
  }
}
void episodes::updateSQL() {
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
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.projectId == coreSet::getSet().projectName().first)
          .order_by(table.episodes.asc())
  )) {
    auto eps = std::make_shared<episodes>();
    eps->p_int_episodes = row.episodes;
    eps->idP = row.id;
    eps->p_prj = row.projectId;
    list.push_back(eps);
  }
  return list;
}

void episodes::setEpisdes(const int64_t &value) {
  p_int_episodes = value;
}

int64_t episodes::getEpisdes() const {
  return p_int_episodes;
}

dstring episodes::getEpisdes_str() const {
  boost::format str("ep%03i");
  str % p_int_episodes;
  return str.str();
}

CORE_NAMESPACE_E
