//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <FileSystemServer.grpc.pb.h>

namespace doodle {

class DOODLELIB_API RpcFileSystemClient {
  std::unique_ptr<FileSystemServer::Stub> p_stub;
  // std::shared_ptr<grpc::Channel> p_channel;

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;

  explicit RpcFileSystemClient(const std::shared_ptr<grpc::Channel>& in_channel);
  /**
   * @brief 获得远程服务器中的文件的基本信息
   * 
   * @param path 
   * @return std::tuple<std::size_t,  bool,   time_point,  bool> 
   *                      大小        是否存在   最后写入时间  是否是文件夹
   */
  std::tuple<std::size_t, bool, time_point, bool> GetInfo(const FSys::path& path);

  std::size_t GetSize(const FSys::path& path);
  bool IsFolder(const FSys::path& path);
  time_point GetTimestamp(const FSys::path& path);
  bool IsExist(const FSys::path& path);
  bool Download(const FSys::path& path);
  bool Upload(const FSys::path& path);
};

}  // namespace doodle