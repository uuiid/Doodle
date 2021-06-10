//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemClient.h"

#include <DoodleLib/Exception/Exception.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
RpcFileSystemClient::RpcFileSystemClient(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(FileSystemServer::NewStub(in_channel))
// p_channel(in_channel)
{
}

std::tuple<std::size_t, bool, std::chrono::time_point<std::chrono::system_clock>, bool> RpcFileSystemClient::GetInfo(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  FileInfo k_out_info;
  auto status = p_stub->GetInfo(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw DoodleError{status.error_message()};

  auto k_t = std::chrono::system_clock::from_time_t(google::protobuf::util::TimeUtil::TimestampToTimeT(k_out_info.update_time()));
  return {k_out_info.size(), k_out_info.isfolder(), k_t, k_out_info.exist()};
}

std::size_t RpcFileSystemClient::GetSize(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  FileInfo k_out_info;
  auto status = p_stub->GetSize(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw DoodleError{status.error_message()};

  return k_out_info.size();
}

std::tuple<bool, bool> RpcFileSystemClient::IsFolder(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  FileInfo k_out_info;
  auto status = p_stub->IsFolder(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw DoodleError{status.error_message()};

  return {k_out_info.exist(), k_out_info.isfolder()};
}

std::chrono::time_point<std::chrono::system_clock> RpcFileSystemClient::GetTimestamp(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  FileInfo k_out_info;
  auto status = p_stub->GetTimestamp(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw DoodleError{status.error_message()};
  auto k_t = std::chrono::system_clock::from_time_t(google::protobuf::util::TimeUtil::TimestampToTimeT(k_out_info.update_time()));

  return k_t;
}

bool RpcFileSystemClient::IsExist(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  FileInfo k_out_info;
  auto status = p_stub->IsExist(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw DoodleError{status.error_message()};

  return k_out_info.exist();
}

bool RpcFileSystemClient::Download(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  if (FSys::exists(in_local_path.parent_path()))
    FSys::create_directories(in_local_path.parent_path());

  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  FileStream k_out_info{};

  FSys::ofstream k_file{in_local_path, std::ios::out | std::ios::binary};
  if (!k_file)
    throw DoodleError{"not create file"};

  k_in_info.set_path(in_server_path.generic_string());
  auto k_out = p_stub->Download(&k_context, k_in_info);

  while (k_out->Read(&k_out_info)) {
    auto& str = k_out_info.data().value();
    k_file.write(str.data(), str.size());
  }

  auto status = k_out->Finish();

  if (!status.ok())
    throw DoodleError{status.error_message()};

  return true;
}

bool RpcFileSystemClient::Upload(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  FileStream k_in_info{};
  FileInfo k_out_info{};

  FSys::ifstream k_file{in_local_path, std::ios::in | std::ios::binary};
  if (!k_file)
    throw DoodleError{"not read file"};

  k_in_info.mutable_info()->set_path(in_server_path.generic_string());
  auto k_in = p_stub->Upload(&k_context, &k_out_info);

  k_in->Write(k_in_info);
  std::string k_value{};
  static std::size_t s_size = 3 * 1024 * 1024;
  k_value.resize(s_size);

  while (k_file) {
    k_file.read(k_value.data(), s_size);
    auto k_s = k_file.gcount();
    if (k_s != s_size)
      k_value.resize(k_s);

    k_in_info.mutable_data()->set_value(std::move(k_value));
    if (k_in->Write(k_in_info))
      throw DoodleError{"write strame errors"};
  }

  return {};
}
}  // namespace doodle