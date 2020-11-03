#pragma once

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING 1
#define _WIN32_WINNT 0x0601

#include <QtCore/qglobal.h>
#include <QSqlQuery>
#include <QSharedPointer>
#include <QMap>
#include <QFileInfo>
#include <QVector>



#if defined(CORE_LIBRARY)
#  define CORE_EXPORT __declspec(dllexport)
#else
#  define CORE_EXPORT __declspec(dllimport)
#endif

#define CORE_NAMESPACE_S namespace doCore {
#define CORE_NAMESPACE_E };


#define DOODLE_FFMPEG_PATH "tools/ffmpeg/bin"

CORE_NAMESPACE_S
//使用一些其他方便的引用类型
typedef QVector<QFileInfo> QfileInfoVector;

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


using assInfoPtrList = QVector<assInfoPtr>;
using fileSqlInfoPtr = QSharedPointer<fileSqlInfo>;

struct synPath_struct;
using synPathListPtr=QVector<synPath_struct>;

class fileArchive;
using fileArchivePtr = std::shared_ptr<fileArchive>;
class mayaArchive;
using mayaArchivePtr = std::shared_ptr<mayaArchive>;

class mayaArchiveShotFbx;
using mayaArchiveShotFbxPtr = std::shared_ptr<mayaArchiveShotFbx>;

using stringList = std::vector<QString>;


CORE_NAMESPACE_E


