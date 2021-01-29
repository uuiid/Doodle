#include "ShotModifySQLDate.h"
#include <corelib/Exception/Exception.h>

#include <corelib/coreOrm/basefile_sqlOrm.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <corelib/shots/episodes.h>
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>

#include <chrono>
// /*保护data里面的宏__我他妈的*/
// #ifdef min
// #undef min
// #endif
// #ifdef max
// #undef max
// #endif
// #include <date/date.h>
// /*保护data里面的宏__我他妈的*/

DOODLE_NAMESPACE_S

ShotModifySQLDate::ShotModifySQLDate(std::weak_ptr<episodes> &eps)
    : p_eps(std::move(eps)) {
}

void ShotModifySQLDate::selectModify() {
  if (p_eps.expired()) throw nullptr_error("episodes nullptr");
  auto k_eps = p_eps.lock();

  int64_t id{0};
  if (k_eps->isInsert())
    id = k_eps->getIdP();
  else
    throw insert_error_info("eps not insert");

  Basefile tab{};

  auto hours = std::chrono::hours(24 * 3);
  auto now   = std::chrono::system_clock().now() - hours;

  auto db    = coreSql::getCoreSql().getConnection();
  auto query = sqlpp::select(tab.shotsId)
                   .from(tab)
                   .where(tab.episodesId == id and tab.filetime >= now)
                   .flags(sqlpp::distinct);

  for (auto &&row : db->run(query)) {
    row.shotsId.value();
    if (!row.shotsId.is_null())
      p_set_id.insert(row.shotsId.value());
  }
}
DOODLE_NAMESPACE_E