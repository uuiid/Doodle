#pragma once

#include <doodlelib_export.h>

#define SPDLOG_FUNCTION static_cast<const char *>(__FUNCSIG__)

#include <DoodleLib/libWarp/sqlppWarp.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/filesystem.hpp>
//#include <ghc/filesystem.hpp>
#include <DoodleLib/Logger/Logger.h>

#include <stdexcept>
//namespace fmt {
//namespace FSys = std::filesystem;
//template <class Char>
//struct formatter<FSys::path, Char> : formatter<basic_string_view<Char>, Char> {
//  template <typename FormatContext>
//  auto format(const FSys::path &in_path, FormatContext &ctx) {
//    if constexpr (std::is_same_v<Char, char>)
//      return formatter<basic_string_view<Char>, Char>::format(in_path.generic_string(), ctx);
//    else if constexpr (std::is_same_v<Char, wchar_t>)
//      return formatter<basic_string_view<Char>, Char>::format(in_path.generic_wstring(), ctx);
//  }
//};
//}  // namespace fmt

#include <Windows.h>

//开始我们的名称空间
namespace doodle {
namespace details {
class no_copy {
 public:
  no_copy()                = default;
  no_copy(const no_copy &) = delete;
  no_copy &operator=(const no_copy &) = delete;
};

/**
 * 这个是判断指针或者共享指针是什么类的帮助函数
 * @tparam T 是否是这个类
 * @tparam RT 传入的指针类型
 * @param in_rt 输入的指针
 * @return 是否是可以转换的
 */
template <class T, class RT>
bool is_class(const RT &in_rt) {
  if (!in_rt)
    return false;
  const auto &k_item = *in_rt;
  return typeid(T) == typeid(k_item);
}
}  // namespace details

namespace FSys {

using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;

inline std::time_t last_write_time_t(const path &in_path) {
  auto k_h = CreateFile(in_path.generic_wstring().c_str(), 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (k_h == INVALID_HANDLE_VALUE)
    throw std::runtime_error{"无效的文件路径"};
  FILETIME k_f_l;
  if (!GetFileTime(k_h, nullptr, nullptr, &k_f_l)) {
    throw std::runtime_error{"无法获得写入时间"};
  }
  ULARGE_INTEGER ull{};
  ull.LowPart  = k_f_l.dwLowDateTime;
  ull.HighPart = k_f_l.dwHighDateTime;

  return static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

inline std::chrono::time_point<std::chrono::system_clock> last_write_time_point(const path &in_path) {
  return std::chrono::system_clock::from_time_t(last_write_time_t(in_path));
}

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
class DragFilesFactory;
class action;
class upload_dir_action;
class upload_file_action;
class upload_dir_and_file_action;
class DoodleLib;
class ThreadPool;

using MetadataPtr               = std::shared_ptr<Metadata>;
using MetadataConstPtr          = std::shared_ptr<const Metadata>;
using RpcMetadataClientPtr      = std::shared_ptr<RpcMetadataClient>;
using RpcMetadataServerPtr      = std::shared_ptr<RpcMetadaataServer>;
using RpcServerHandlePtr        = std::shared_ptr<RpcServerHandle>;
using RpcFileSystemServerPtr    = std::shared_ptr<RpcFileSystemServer>;
using RpcFileSystemClientPtr    = std::shared_ptr<RpcFileSystemClient>;
using ProjectPtr                = std::shared_ptr<Project>;
using EpisodesPtr               = std::shared_ptr<Episodes>;
using ShotPtr                   = std::shared_ptr<Shot>;
using AssetsPtr                 = std::shared_ptr<Assets>;
using AssetsFilePtr             = std::shared_ptr<AssetsFile>;
using coreSqlPtr                = std::shared_ptr<CoreSql>;
using LabelNodePtr              = std::shared_ptr<LabelNode>;
using AssetsFilePtr             = std::shared_ptr<AssetsFile>;
using MetadataFactoryPtr        = std::shared_ptr<MetadataFactory>;
using TimeDurationPtr           = std::shared_ptr<TimeDuration>;
using CommentPtr                = std::shared_ptr<Comment>;
using AssetsPathPtr             = std::shared_ptr<AssetsPath>;
using DragFilesFactoryPtr       = std::shared_ptr<DragFilesFactory>;
using action_ptr                = std::shared_ptr<action>;
using UploadDirActionPtr        = std::shared_ptr<upload_dir_action>;
using UploadFileActionPtr       = std::shared_ptr<upload_file_action>;
using UploadDirAndFileActionPre = std::shared_ptr<upload_dir_and_file_action>;
using DoodleLibPtr              = std::unique_ptr<DoodleLib>;
using ThreadPoolPtr             = std::shared_ptr<ThreadPool>;
class Doodle;
[[maybe_unused]] DOODLELIB_API DoodleLibPtr make_doodle_lib();
template <typename SSC, typename SSN>
SSC ConvStr(const SSN &str) {
  static_assert(false, "这个函数会出错");
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

}  // namespace doodle

wxDECLARE_APP(doodle::Doodle);
