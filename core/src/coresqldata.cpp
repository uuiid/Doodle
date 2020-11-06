#include "coresqldata.h"

#include <QSqlQuery>
#include <QVariant>

CORE_NAMESPACE_S

coresqldata::coresqldata() {
  idP = -1;
}

qint64 coresqldata::getIdP() const {
  if (idP > 0) return idP;
  else return 0;
}

bool coresqldata::isNULL() const {
  return idP <= 0;
}

CORE_NAMESPACE_E
