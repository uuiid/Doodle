#pragma once

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#include <wx/wx.h>
#ifdef _CRT_SECURE_NO_WARNINGS
#undef _CRT_SECURE_NO_WARNINGS
#endif
//<wx/wx.h># 这个头要在wx的前面才不会出错
//<boost/process.hpp> # 因为wx中的setup.h 定义了一个pid_t宏 会影响boost中的处理
#ifdef _
#undef _
#endif
#ifdef pid_t
#undef pid_t
#endif

#include <doodlelib_export.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream_buffer.hpp>


#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#include <wx/wx.h>
#ifdef _CRT_SECURE_NO_WARNINGS
#undef _CRT_SECURE_NO_WARNINGS
#endif
//<wx/wx.h># 这个头要在wx的前面才不会出错
//<boost/process.hpp> # 因为wx中的setup.h 定义了一个pid_t宏 会影响boost中的处理
#ifdef _
#undef _
#endif
#ifdef pid_t
#undef pid_t
#endif

#define DOODLE_NAMESPACE doodle
#define DOODLE_NAMESPACE_S namespace DOODLE_NAMESPACE {
#define DOODLE_NAMESPACE_E \
  }                        \
  ;

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

#define DOODLE_UE_PATH "Engine/Binaries/Win64/UE4Editor.exe"

#define DOODLE_STATIC(getValueName, type_, value) \
  namespace staticValue {                         \
  type_ getValueName() {                          \
    const static type_ install{value};            \
    return install;                               \
  };                                              \
  };
#define DOODLE_STR_STATIC(getValueName, value) \
  DOODLE_STATIC(getValueName, std::string, #value)
#define DOODLE_STR_S(value) \
  DOODLE_STR_STATIC(value##Obj, value)

#define DOODLE_STATIC_DECLARE(getValueName, type_, value) \
  namespace staticValue {                                 \
  type_ getValueName();                                   \
  };

#define DOODLE_STR_STATIC_DECLARE(getValueName, value) \
  DOODLE_STATIC_DECLARE(getValueName, std::string, #value)
#define DOODLE_STR_S_DECLARE(value) \
  DOODLE_STR_STATIC_DECLARE(value##Obj, value)

//添加资源
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(DoodleLibResource);

// 添加数据库连接项
namespace sqlpp::mysql {
class connection;
struct connection_config;
}  // namespace sqlpp::mysql


#include <date/date.h>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
namespace cereal{

template <class Archive> inline
void save(Archive &ar, boost::filesystem::path const &path) {
  ar(cereal::make_nvp("path",path.generic_string()));
}
template <class Archive> inline
void load(Archive &ar, boost::filesystem::path &path) {
  std::string str{};
  ar(str);
  path = boost::filesystem::path{str};

};

template <class Archive>
std::int32_t save_minimal(Archive const & ar, date::year const& in_year){
  return (int)in_year;
}
template <class Archive>
void load_minimal(Archive const &, date::year &in_year, std::int32_t const &value) {
  in_year = date::year{value};
}

template <class Archive>
std::uint32_t save_minimal(Archive const& ar, date::month const& in_month){
  return (std::uint32_t)in_month;
}
template <class Archive>
void load_minimal(Archive const &, date::month &in_month, std::uint32_t const &value) {
  in_month = date::month{value};
}

template <class Archive>
std::uint32_t save_minimal(Archive const& ar, date::day const& in_day){
  return (std::uint32_t)in_day;
}
template <class Archive>
void load_minimal(Archive const &, date::day &in_day, std::uint32_t const &value) {
  in_day = date::day{value};
}
}

// namespace std::filesystem {
// template <class Archive>
// std::string save_minimal(Archive const &, std::filesystem::path const &path) {
//   return path.generic_string();
// }
// template <class Archive>
// void load_minimal(Archive const &, std::filesystem::path &path, std::string const &value) {
//   path = value;
// };
// }  // namespace std::filesystem

//开始我们的名称空间
namespace doodle {

namespace FSys {
using namespace boost::filesystem;
// using fstream  = std::fstream;
// using istream  = std::istream;
// using ifstream = std::ifstream;
// using ofstream = std::ofstream;
// using ostream  = std::ostream;
}  // namespace FSys

using ConnPtr = std::unique_ptr<sqlpp::mysql::connection>;

class CoreSet;
class Project;
class Episodes;
class Shot;
class Metadata;
class Assets;
class CoreSql;
class LabelNode;
class AssetsFile;
class MetadataFactory;
class ContextMenu;
class TimeDuration;
class Comment;
class AssetsPath;

class RpcMetadataClient;
class RpcMetadaataServer;
class ProjectManage;
class RpcServerHandle;
class AssetsTree;
class ListAttributeModel;
class RpcFileSystemClient;
class RpcFileSystemServer;

using MetadataPtr            = std::shared_ptr<Metadata>;
using MetadataConstPtr       = std::shared_ptr<const Metadata>;
using RpcMetadataClientPtr   = std::shared_ptr<RpcMetadataClient>;
using RpcMetadataServerPtr   = std::shared_ptr<RpcMetadaataServer>;
using RpcServerHandlePtr     = std::shared_ptr<RpcServerHandle>;
using RpcFileSystemServerPtr = std::shared_ptr<RpcFileSystemServer>;
using RpcFileSystemClientPtr = std::shared_ptr<RpcFileSystemClient>;
using ProjectPtr             = std::shared_ptr<Project>;
using EpisodesPtr            = std::shared_ptr<Episodes>;
using ShotPtr                = std::shared_ptr<Shot>;
using AssetsPtr              = std::shared_ptr<Assets>;
using AssetsFilePtr          = std::shared_ptr<AssetsFile>;
using coreSqlPtr             = std::shared_ptr<CoreSql>;
using LabelNodePtr           = std::shared_ptr<LabelNode>;
using AssetsFilePtr          = std::shared_ptr<AssetsFile>;
using MetadataFactoryPtr     = std::shared_ptr<MetadataFactory>;
using TimeDurationPtr        = std::shared_ptr<TimeDuration>;
using CommentPtr             = std::shared_ptr<Comment>;
using AssetsPathPtr          = std::shared_ptr<AssetsPath>;
class Doodle;
enum class filterState {
  useFilter,
  notFilter,
  showAll,
};

template <typename SSC, typename SSN>
SSC ConvStr(const SSN &str) {
  return SSC{str};
}

template <>
std::string ConvStr(const wxString &str);

template <>
wxString ConvStr(const std::string &str);

template <>
wxString ConvStr(const FSys::path &str);

template <>
FSys::path ConvStr(const wxString &str);

//模板特化一个指针类型的模板
template <typename SSC, typename SSN>
SSC ConvStr(const SSN *str) {
  return SSC{str};
}
//继续特化一个char*的平常用的
template <>
wxString ConvStr(const char *(str));

//template <typename SSC,typename SSN,std::size_t N>
//SSC ConvStr(const SSN (&str)[N]){
//  return  SSC{str};
//}
//
//继续特化一个char*的平常用的
template <std::size_t N>
wxString ConvStr(const char (&str)[N]) {
  return wxString::FromUTF8(str, N);
};

}

wxDECLARE_APP(doodle::Doodle);
