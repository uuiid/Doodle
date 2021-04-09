#pragma once

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING 1
#define _WIN32_WINNT 0x0A00

#include <boost/filesystem.hpp>
#include <core_export.h>
#include <cereal/access.hpp>

#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

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

#define DOODLE_CONTENT "Content"
#define DOODLE_EPFORMAT "ep%03i"
#define DOODLE_SHFORMAT "sc%04i"
#define DOODLE_UE_PATH "Engine/Binaries/Win64/UE4Editor.exe"
//添加资源
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(CoreResource);
namespace sqlpp::sqlite3 {
class connection;
struct connection_config;
}  // namespace sqlpp::sqlite3

namespace boost::filesystem {
class path;
template <class Archive>
void save(Archive &archive,
          boost::filesystem::path const &path_) {
  archive(path_.generic_string());
}

template <class Archive>
void load(Archive &archive,
          boost::filesystem::path &path_) {
  std::string str;
  archive(str);
  path_ = path{str};
}
}  // namespace boost::filesystem




//开始我们的名称空间
DOODLE_NAMESPACE_S
namespace FSys    = boost::filesystem;
namespace fileSys = boost::filesystem;

using pathPtr = std::shared_ptr<FSys::path>;

const static uint64_t off{8000000};
using ConnPtr = std::unique_ptr<sqlpp::sqlite3::connection>;

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

using dstring = std::string;

using dstringList = std::vector<std::string>;

using dpathPtr     = std::shared_ptr<fileSys::path>;
using dpathListPtr = std::vector<dpathPtr>;
using dpathList    = std::vector<fileSys::path>;

class Project;
using ProjectPtr = std::shared_ptr<Project>;

DOODLE_NAMESPACE_E
