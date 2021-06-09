//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemClient.h"

namespace doodle {
RpcFileSystemClient::RpcFileSystemClient(const std::shared_ptr<grpc::Channel>& in_channel) {
}

std::tuple<std::size_t, bool, std::chrono::time_point<std::chrono::system_clock>, bool> RpcFileSystemClient::GetInfo(const FSys::path& path) {
  return {};
}

std::size_t RpcFileSystemClient::GetSize(const FSys::path& path) {
  return {};
}

bool RpcFileSystemClient::IsFolder(const FSys::path& path) {
  return {};
}

std::chrono::time_point<std::chrono::system_clock> RpcFileSystemClient::GetTimestamp(const FSys::path& path) {
  return {};
}

bool RpcFileSystemClient::IsExist(const FSys::path& path) {
  return {};
}

bool RpcFileSystemClient::Download(const FSys::path& path) {
  return {};
}

bool RpcFileSystemClient::Upload(const FSys::path& path) {
  return {};
}
}  // namespace doodle