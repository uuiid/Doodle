#pragma once

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING 1
#define _WIN32_WINNT 0x0A00

#include <QtCore/qglobal.h>
// #include <QtCore/qobject.h>
// #include <QtCore/QMetaObject>
#include <QtCore/QString>

#include <vector>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <variant>
#include <regex>
#include <boost/core/noncopyable.hpp>
#include <boost/filesystem.hpp>
#if defined(CORE_LIBRARY)
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif

#define DOODLE_NAMESPACE doodle
#define DOODLE_NAMESPACE_S namespace DOODLE_NAMESPACE {
#define DOODLE_NAMESPACE_E \
  }                        \
  ;

#define DOODLE_TOS(str) (#str)
#define DOODLE_TOS_(str) #str
#define DOODLE_RTTE_CLASS(nameSpace, className) (DOODLE_TOS_(nameSpace) "::" DOODLE_TOS_(className))

#define DOCORE_RTTE_CLASS(className) DOODLE_RTTE_CLASS(DOODLE_NAMESPACE, className)

#define DOODLE_INSRANCE(className) static std::unordered_set<className *> p_instance
#define DOODLE_INSRANCE_CPP(className) \
  std::unordered_set<className *> className::p_instance {}

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

#define DOODLE_FFMPEG_PATH "tools/ffmpeg/bin"
#define DOODLE_BACKUP "backup"
#define DOODLE_CONTENT "Content"
#define DOODLE_EPFORMAT "ep%03i"
#define DOODLE_SHFORMAT "sc%04i"

#if __has_cpp_attribute(nodiscard) && \
    !(defined(__clang__) && (__cplusplus < 201703L))
#define DOODLE_NODISCARD [[nodiscard]]

#endif
#if __has_cpp_attribute(no_unique_address) && \
    !(defined(__GNUC__) && (__cplusplus < 201100))
#define DOODLE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#ifndef DOODLE_NODISCARD
#define DOODLE_NODISCARD
#endif
#ifndef DOODLE_NO_UNIQUE_ADDRESS
#define DOODLE_NO_UNIQUE_ADDRESS
#endif

namespace sqlpp::mysql {
class connection;
struct connection_config;
}  // namespace sqlpp::mysql

namespace boost::filesystem {
class path;
}

class QFileInfo;

DOODLE_NAMESPACE_S
namespace fileSys = boost::filesystem;
const static uint64_t off{8000000};
using mysqlConnPtr = std::unique_ptr<sqlpp::mysql::connection>;

//使用一些其他方便的引用类型

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

class movieEpsArchive;
class synData;
class pathParsing;

class ShotModifySQLDate;
class queueData;
using queueDataPtr = std::shared_ptr<queueData>;
//评论类
class CommentInfo;
using CommentInfoPtr = std::shared_ptr<CommentInfo>;

//共享指针引用类
using shotPtr      = std::shared_ptr<shot>;
using episodesPtr  = std::shared_ptr<episodes>;
using shotClassPtr = std::shared_ptr<shotClass>;
using shotTypePtr  = std::shared_ptr<shotType>;
using shotInfoPtr  = std::shared_ptr<shotFileSqlInfo>;

using assDepPtr   = std::shared_ptr<assdepartment>;
using assClassPtr = std::shared_ptr<assClass>;
using znchNamePtr = std::shared_ptr<znchName>;
using assTypePtr  = std::shared_ptr<assType>;
using assInfoPtr  = std::shared_ptr<assFileSqlInfo>;

using fileSqlInfoPtr = std::shared_ptr<fileSqlInfo>;
using synDataPtr     = std::shared_ptr<synData>;

using pathParsingPtr = std::shared_ptr<pathParsing>;
//列表引用类

using synDataPtrList   = std::vector<synDataPtr>;
using episodesPtrList  = std::vector<episodesPtr>;
using shotPtrList      = std::vector<shotPtr>;
using shotClassPtrList = std::vector<shotClassPtr>;
using shotTypePtrList  = std::vector<shotTypePtr>;
using shotInfoPtrList  = std::vector<shotInfoPtr>;
using assDepPtrList    = std::vector<assDepPtr>;

using assClassPtrList = std::vector<assClassPtr>;
using assTypePtrList  = std::vector<assTypePtr>;
using assInfoPtrList  = std::vector<assInfoPtr>;
struct synPath_struct;
using synPath_structPtr = std::shared_ptr<synPath_struct>;
using synPathListPtr    = std::vector<synPath_struct>;
class fileArchive;

using fileArchivePtr = std::shared_ptr<fileArchive>;
class mayaArchive;
using mayaArchivePtr = std::shared_ptr<mayaArchive>;
class mayaArchiveShotFbx;

using mayaArchiveShotFbxPtr = std::shared_ptr<mayaArchiveShotFbx>;

using dstring    = std::string;
using stringList = std::vector<QString>;

using dstringList = std::vector<std::string>;

using dpath     = boost::filesystem::path;
using dpathPtr  = std::shared_ptr<dpath>;
using dpathList = std::vector<dpath>;

using dataInfoPtr = std::variant<
    episodesPtr,
    shotPtr,
    shotClassPtr,
    shotTypePtr,
    shotInfoPtr,

    assDepPtr,
    assClassPtr,
    assTypePtr,
    znchNamePtr,
    assInfoPtr,

    queueDataPtr>;
DOODLE_NAMESPACE_E

// Q_DECLARE_METATYPE( shotInfoPtr)
