#include "ShotModifySQLDate.h"
#include <src/Exception/Exception.h>

#include <src/coreOrm/basefile_sqlOrm.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <src/shots/episodes.h>
#include <src/core/coreset.h>
#include <src/core/coresql.h>

DOODLE_NAMESPACE_S

ShotModifySQLDate::ShotModifySQLDate(std::weak_ptr<episodes> &eps)
    : p_eps(std::move(eps)) {
}

void ShotModifySQLDate::selectModify() {
  if (p_eps.expired()) throw nullptr_error("episodes nullptr");
  auto k_eps = p_eps.lock();
  if (k_eps->isInsert())
    auto id = k_eps->getIdP();
  else
    throw insert_error_info("eps not insert");

  Basefile tab{};

  auto db    = coreSql::getCoreSql().getConnection();
  auto query = sqlpp::select(tab.shotsId)
                   .where(tab.id == id and tab.filetime >= "2021-01-19")
                   .flags(sqlpp::distinct);
  for (auto &&row : db->run(query)) {
    row.shotsId.value();
    if (!row.shotsId.is_null())
      p_set_id.insert(row.shotsId.value());
  }
}
DOODLE_NAMESPACE_E