//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/libWarp/protobuf_warp.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <FileSystemServer.grpc.pb.h>

#include <optional>

//#include "../../../cmake-build-debug-vs2019/src/DoodleLib/FileSystemServer.grpc.pb.h"

namespace doodle {

//class equal_fun {
// public:
//  bool operator()(const std::tuple<std::optional<bool>, std::optional<bool> >& arg) {
//    return;
//  };
//};
//class is_down_fun {
//};
/**
 * @brief 这个类在上传和下载时会`先比较文件，当比较成功后， 不一致才会上传和下载
 *
 */
class DOODLELIB_API RpcFileSystemClient : public details::no_copy {
  std::unique_ptr<FileSystemServer::Stub> p_stub;
  std::recursive_mutex p_mutex;
  // std::shared_ptr<grpc::Channel> p_channel;
  /**
   * 这个是用来比较文件的函数
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   * @return std::tuple<std::optional<bool>,  是否相等
   *                    std::optional<bool>,  是否需要下载
   *                    std::optional<bool>,  服务器文件是否存在
   *                   > 
   */
  std::tuple<std::optional<bool>, std::optional<bool>, bool > compare_file_is_down(const FSys::path& in_local_path, const FSys::path& in_server_path);
  /**
   * @brief 这个是递归调用进行下载
   * 
   * @param in_local_path 本地路径
   * @param in_server_path  服务器路径
   * @param k_future_list 等待结果
   */
  void _DownloadDir(const FSys::path& in_local_path, const FSys::path& in_server_path, std::vector<std::future<void> >& k_future_list);
  /**
   * @brief 递归调用进行上传
   * 
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   * @param in_backup_path 备份路径
   * @param in_future_list 等待结果
   */
  void _UploadDir(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path, std::vector<std::future<void> >& in_future_list);

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;
  /**
   *@brief 这个时用来判读是否进行同步或者下载的函子别名
   * std::tuple<std::optional<bool>,  是否相等
   *            std::optional<bool> > 是否需要下载
   */
  using syn_fun = std::function<bool(const std::tuple<std::optional<bool>, std::optional<bool> >& arg)>;

  explicit RpcFileSystemClient(const std::shared_ptr<grpc::Channel>& in_channel);
  /**
   * @brief 获得远程服务器中的文件的基本信息
   * 
   * @param path 
   * @return std::tuple<
   *                   std::size_t,  文件大小
   *                   bool,         是存在
   *                   time_point,   最后写入时间
   *                   bool          是文件夹
   *                   >
   */
  std::tuple<std::size_t, bool, time_point, bool> GetInfo(const FSys::path& in_server_path);

  std::size_t GetSize(const FSys::path& in_server_path);
  /**
   * @brief 判断是否是文件夹
   * 
   * @param in_server_path 服务器路径
   * @return std::tuple<
   *                   bool,是存在
   *                   bool 是是文件夹
   *                   >
   */
  std::tuple<bool, bool> IsFolder(const FSys::path& in_server_path);
  time_point GetTimestamp(const FSys::path& in_server_path);
  bool IsExist(const FSys::path& in_server_path);
  /**
   * @brief 下载服务器中的文件
   * 
   * @param in_local_path 本地文件路径
   * @param in_server_path 服务器中的文件路径
   * @return true 下载完成
   * @return false 下载失败
   */
  void Download(const FSys::path& in_local_path, const FSys::path& in_server_path);
  /**
   * @brief 将文件上传到服务器中
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   * @param in_backup_path 上传时的备份路径， 如果上传文件夹， 备份文件夹也应该是文件夹， 文件则对应文件
   * @return 上传结果
   */
  void Upload(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path = {});

  void DownloadDir(const FSys::path& in_local_path, const FSys::path& in_server_path);
  void UploadDir(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path = {});

  void DownloadFile(const FSys::path& in_local_path, const FSys::path& in_server_path);
  void UploadFile(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path = {});
  // TODO: 要将比较函数提取为函子, 作为同步功能的基础
  //  void DownloadFile(const FSys::path& in_local_path, const FSys::path& in_server_path,const syn_fun& in_syn_fun );
  //  void UploadFile(const FSys::path& in_local_path, const FSys::path& in_server_path,const syn_fun& in_syn_fun );
};

}  // namespace doodle
