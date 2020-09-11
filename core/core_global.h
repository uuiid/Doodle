#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QSqlQuery>
#include <QSharedPointer>
#include <QMap>


#if defined(CORE_LIBRARY)
#  define CORE_EXPORT Q_DECL_EXPORT
#else
#  define CORE_EXPORT Q_DECL_IMPORT
#endif

#define CORE_NAMESPACE_S namespace doCore {
#define CORE_DNAMESPACE_E }


typedef QSharedPointer<QSqlQuery> sqlQuertPtr;
typedef QMap<QString,QString> mapStringPtr;



#endif // CORE_GLOBAL_H
