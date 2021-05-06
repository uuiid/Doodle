#pragma once

#include <core_export.h>

#include <boost/filesystem.hpp>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>


#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#define DOODLE_NAMESPACE doodle
#define DOODLE_NAMESPACE_S namespace DOODLE_NAMESPACE {
#define DOODLE_NAMESPACE_E \
  }                        \
  ;

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

#define DOODLE_UE_PATH "Engine/Binaries/Win64/UE4Editor.exe"
//添加资源
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(CoreResource);

// 添加数据库连接项
namespace sqlpp::sqlite3 {
class connection;
struct connection_config;
}  // namespace sqlpp::sqlite3

// 添加boost::filesystem path 序列化储存
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
using ConnPtr = std::unique_ptr<sqlpp::sqlite3::connection>;

class Project;
class Episodes;
class Shot;
class Metadata;
class Assets;
class coreSql;
class LabelNode;

using ProjectPtr = std::shared_ptr<Project>;
using EpisodesPtr = std::shared_ptr<Episodes>;
using ShotPtr = std::shared_ptr<Shot>;
using MetadataPtr = std::shared_ptr<Metadata>;
using AssetsPtr = std::shared_ptr<Assets>;
using coreSqlPtr = std::shared_ptr<coreSql>;
using LabelNodePtr = std::shared_ptr<LabelNode>;

DOODLE_NAMESPACE_E