#include "coresqldata.h"

#include <QSqlQuery>
#include <QVariant>

CORE_NAMESPACE_S

coresqldata::coresqldata() {
  idP = -1;
}

qint64 coresqldata::getIdP() const {
  if (idP >= 0)
    return idP;
  else
    return 0;
}

bool coresqldata::isNULL() const {
  if (idP >= 0)
    return false;
  else
    return true;
}

void coresqldata::getInsertID(sqlQuertPtr &query) {
  if (!query->exec("SELECT LAST_INSERT_ID() as id_;"))
    return;
  if (query->next()) {
    idP = query->value("id_").toInt();
  }
}
CORE_NAMESPACE_E
