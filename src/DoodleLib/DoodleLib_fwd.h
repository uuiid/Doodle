#pragma once

#include <DoodleLib/libWarp/sqlppWarp.h>

#include <boost/filesystem.hpp>
//开始我们的名称空间
namespace doodle {
namespace details {
class no_copy {
 public:
  no_copy()                = default;
  no_copy(const no_copy &) = delete;
  no_copy &operator=(const no_copy &) = delete;
};
}  // namespace details

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
class DragFilesFactory;
class Action;
class UploadDirAction;
class UploadFileAction;
class DoodleLib;
class ThreadPool;

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
using DragFilesFactoryPtr    = std::shared_ptr<DragFilesFactory>;
using ActionPtr              = std::shared_ptr<Action>;
using UploadDirActionPtr     = std::shared_ptr<UploadDirAction>;
using UploadFileActionPtr    = std::shared_ptr<UploadFileAction>;
using DoodleLibPtr           = std::unique_ptr<DoodleLib>;
using ThreadPoolPtr          = std::shared_ptr<ThreadPool>;
class Doodle;
DoodleLibPtr make_doodle_lib();
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

}  // namespace doodle

wxDECLARE_APP(doodle::Doodle);
