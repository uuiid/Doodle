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
  archive(cereal::make_nvp("path_string", path_.generic_string()));
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
namespace FSys = boost::filesystem;

using pathPtr = std::shared_ptr<FSys::path>;

const static uint64_t off{8000000};
using ConnPtr = std::unique_ptr<sqlpp::sqlite3::connection>;

class Project;
class Episodes;
class Shot;
using ProjectPtr  = std::shared_ptr<Project>;
using EpisodesPtr = std::shared_ptr<Episodes>;
using ShotPtr     = std::shared_ptr<Shot>;

DOODLE_NAMESPACE_E
