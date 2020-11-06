#pragma once

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING 1
#define _WIN32_WINNT 0x0A00

#include <QtCore/qglobal.h>

#include <vector>
#include <iostream>
#include <memory>

#if defined(CORE_LIBRARY)
#  define CORE_EXPORT __declspec(dllexport)
#else
#  define CORE_EXPORT __declspec(dllimport)
#endif

#define CORE_NAMESPACE_S namespace doCore {
#define CORE_NAMESPACE_E };

#define DOODLE_FFMPEG_PATH "tools/ffmpeg/bin"
namespace sqlpp::mysql {
class connection;
struct connection_config;
}

namespace boost::filesystem{
class path;
}

class QFileInfo;

CORE_NAMESPACE_S
using mysqlConnPtr = std::unique_ptr<sqlpp::mysql::connection>;


//使用一些其他方便的引用类型
typedef std::vector<QFileInfo> QfileInfoVector;

typedef QMap<QString, QString> mapStringPtr;

class shot;
class episodes;

class shotClass;
class shotType;


class fileSqlInfo;
class shotFileSqlInfo;

class assdepartment;
class assClass;
class znchName;
class assType;
class assFileSqlInfo;

//共享指针引用类
using shotPtr =std::shared_ptr<shot>;
using episodesPtr =std::shared_ptr<episodes>  ;
using shotClassPtr = std::shared_ptr<shotClass>;
using shotTypePtr = std::shared_ptr<shotType>;
using shotInfoPtr = std::shared_ptr<shotFileSqlInfo>;
using assDepPtr = std::shared_ptr<assdepartment>;

using assClassPtr = std::shared_ptr<assClass>;
using znchNamePtr = std::shared_ptr<znchName>;
using assTypePtr = std::shared_ptr<assType>;
using assInfoPtr = std::shared_ptr<assFileSqlInfo>;

//列表引用类
using episodesPtrList = std::vector<episodesPtr>;
using shotPtrList = std::vector<shotPtr>;
using shotClassPtrList = std::vector<shotClassPtr>;
using shotTypePtrList = std::vector<shotTypePtr>;
using shotInfoPtrList = std::vector<shotInfoPtr>;

using assDepPtrList = std::vector<assDepPtr>;
using assClassPtrList = std::vector<assClassPtr>;
using assTypePtrList = std::vector<assTypePtr>;
using assInfoPtrList = std::vector<assInfoPtr>;


using fileSqlInfoPtr = std::shared_ptr<fileSqlInfo>;

struct synPath_struct;
using synPathListPtr = std::vector<synPath_struct>;

class fileArchive;
using fileArchivePtr = std::shared_ptr<fileArchive>;
class mayaArchive;
using mayaArchivePtr = std::shared_ptr<mayaArchive>;

class mayaArchiveShotFbx;
using mayaArchiveShotFbxPtr = std::shared_ptr<mayaArchiveShotFbx>;

using stringList = std::vector<QString>;

using dpath = boost::filesystem::path;
using dstring = std::string;
CORE_NAMESPACE_E



