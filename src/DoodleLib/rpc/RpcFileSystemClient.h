//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <FileSystemServer.grpc.pb.h>
#include <optional>
#include <DoodleLib/threadPool/ThreadPool.h>

//#include "../../../cmake-build-debug-vs2019/src/DoodleLib/FileSystemServer.grpc.pb.h"

namespace doodle {
/**
 * @brief 这个类在上传和下载时会先比较文件，当比较成功后， 不一致才会上传和下载
 *
 */
class DOODLELIB_API RpcFileSystemClient {
  std::unique_ptr<FileSystemServer::Stub> p_stub;
  std::recursive_mutex p_mutex;
  // std::shared_ptr<grpc::Channel> p_channel;
  std::optional<bool> compare_file_is_down(const FSys::path& in_local_path, const FSys::path& in_server_path);
  void _DownloadDir(const FSys::path& in_local_path, const FSys::path& in_server_path, std::vector<std::future<void> >& k_future_list);
  void _UploadDir(const FSys::path& in_local_path, const FSys::path& in_server_path, std::vector<std::future<void> >& in_future_list);

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;

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
   * @return 上传结果
   */
  void Upload(const FSys::path& in_local_path, const FSys::path& in_server_path);

  void DownloadDir(const FSys::path& in_local_path, const FSys::path& in_server_path);
  void UploadDir(const FSys::path& in_local_path, const FSys::path& in_server_path);

  void DownloadFile(const FSys::path& in_local_path, const FSys::path& in_server_path);
  void UploadFile(const FSys::path& in_local_path, const FSys::path& in_server_path);
};

}  // namespace doodle
