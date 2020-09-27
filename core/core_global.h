#pragma once


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




CORE_NAMESPACE_S
typedef QSharedPointer<QSqlQuery> sqlQuertPtr;
typedef QMap<QString,QString> mapStringPtr;

class shot;
class episodes;

class fileClass;
class fileType;
class assType;

class fileSqlInfo;

class shotFileSqlInfo;
class assFileSqlInfo;

class znchName;

//共享指针引用类
typedef QSharedPointer<shot> shotPtr;
typedef QSharedPointer<episodes> episodesPtr;
typedef QSharedPointer<fileClass> fileClassPtr;
typedef QSharedPointer<fileType> fileTypePtr;
typedef QSharedPointer<assType> assTypePtr;
typedef QSharedPointer<shotFileSqlInfo> shotInfoPtr;
typedef QSharedPointer<assFileSqlInfo> assInfoPtr;
typedef QSharedPointer<znchName> znchNamePtr;

//弱指针指针引用类
typedef QWeakPointer<shot> shotPtrW;
typedef QWeakPointer<episodes> episodesPtrW;
typedef QWeakPointer<fileClass> fileClassPtrW;
typedef QWeakPointer<fileType> fileTypePtrW;
typedef QWeakPointer<assType> assTypePtrW;
typedef QWeakPointer<shotFileSqlInfo> shotlInfoPtrW;
typedef QWeakPointer<assFileSqlInfo> assInfoPtrW;
typedef QWeakPointer<znchName> znchNamePtrW;

//列表引用类
typedef QVector<episodesPtr>  episodesPtrList;
typedef QVector<shotPtr>      shotPtrList;
typedef QVector<fileClassPtr> fileClassPtrList;
typedef QVector<fileTypePtr>  fileTypePtrList;
typedef QVector<assTypePtr>   assTypePtrList;

typedef QVector<shotInfoPtr> shotInfoPtrList;
typedef QVector<assInfoPtr>  assInfoPtrList;


CORE_DNAMESPACE_E
