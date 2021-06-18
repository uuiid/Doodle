//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemClient.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <Logger/Logger.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
std::optional<bool> RpcFileSystemClient::compare_file_is_down(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  auto k_l_ex                            = FSys::exists(in_local_path);
  auto k_l_dir                           = k_l_ex ? FSys::is_directory(in_local_path) : false;
  auto k_l_sz                            = k_l_ex ? FSys::file_size(in_local_path) : 0;
  auto k_l_ti                            = k_l_ex ? std::chrono::system_clock::from_time_t(FSys::last_write_time(in_local_path)) : std::chrono::time_point<std::chrono::system_clock>{};
  auto [k_s_sz, k_s_ex, k_s_ti, k_s_dir] = GetInfo(in_server_path);

  if (k_l_ex && k_s_ex) {      ///本地文件和服务器文件都存在
    if (k_l_dir || k_s_dir) {  ///两个任意一个为目录我们都没有办法确定上传和下载的方案
      return {};
    }
    if (k_l_sz == k_s_sz &&
        k_l_ti == k_s_ti) {  ///我们比较两个文件的时间和大小都相同的时候， 直接表示两个文件相同， 既不上穿也不下载
      return {};
    } else {
      if (k_l_ti < k_s_ti) {  ///本地文件的修改时间小于服务器时间 那么就是本地文件比较旧 服务器文件新， 需要下载
        return {true};
      } else {  ///本地文件的修改时间大于服务器时间 那么就是本地文件比较新 服务时间比较旧, 需要上传
        return {false};
      }
    }
  } else if (k_l_ex) {  ///本地文件存在和服务器文件不存在
    return {false};
  } else if (k_s_ex) {  ///本地文件不存在和服务器存在
    return {true};
  } else {  /// 本地和服务器文件都不存在
    return {};
  }
}

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

void RpcFileSystemClient::Download(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  auto [k_ex, k_dir] = IsFolder(in_server_path);
  if (!k_ex)
    throw DoodleError{"服务器中不存在文件或者目录"};

  if (k_dir) {
    DownloadDir(in_local_path, in_server_path);
  } else {
    DownloadFile(in_local_path, in_server_path);
  }
}

void RpcFileSystemClient::Upload(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  if (!FSys::exists(in_local_path))
    throw DoodleError{"本地中不存在文件或者目录"};

  if (FSys::is_directory(in_local_path))
    UploadDir(in_local_path, in_server_path);
  else
    UploadFile(in_local_path, in_server_path);
}

void RpcFileSystemClient::DownloadDir(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  if (!FSys::exists(in_local_path))
    FSys::create_directories(in_local_path);

  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  FileInfo k_out_info{};

  k_in_info.set_path(in_local_path.generic_string());
  auto k_out = p_stub->GetList(&k_context, k_in_info);

  std::vector<std::pair<FSys::path,FSys::path> > path_list{};
  auto k_prot = DoodleLib::Get().get_thread_pool();
  std::vector<std::future<void>> k_list;

  while (k_out->Read(&k_out_info)) {
    FSys::path k_s_p = k_out_info.path();
    FSys::path k_l_p = in_local_path / k_s_p.filename();
    if (k_out_info.isfolder())
      k_list.emplace_back(
                k_prot->enqueue(
                    [k_s_p,k_l_p,this](){ DownloadDir(k_s_p,k_l_p);}
              )
                );
    else
      k_list.emplace_back(
          k_prot->enqueue(
              [k_s_p,k_l_p,this](){ DownloadFile(k_s_p,k_l_p);}
              )
          );
  }

  std::future_status k_status;
  auto k_it = k_list.begin();
  while (!k_list.empty()){
    k_status = k_it->wait_for(std::chrono::milliseconds{100});
    if(k_status == std::future_status::ready){
      try {
        k_it->get();
      } catch (const std::runtime_error& error) {
        DOODLE_LOG_DEBUG(error.what());
      }
      k_it = k_list.erase(k_it);
    } else{
      ++k_it;
    }
    if(k_it == k_list.end()){
      k_it = k_list.begin();
    }
  }


}
void RpcFileSystemClient::UploadDir(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  if (!FSys::exists(in_local_path))
    throw DoodleError{"未找到上传文件夹"};

  grpc::ClientContext k_context{};

  FileInfo k_in_info{};
  FileInfo k_out_info{};

  std::vector<std::pair<FSys::path,FSys::path> > path_list{};
  auto k_prot = DoodleLib::Get().get_thread_pool();
  std::vector<std::future<void>> k_list;

  for (const auto& k_it : FSys::directory_iterator(in_local_path)) {
    FSys::path k_s_p = in_server_path / k_it.path().filename();
    const FSys::path& k_l_p= k_it.path();

    if(FSys::is_directory(k_it)){
      k_list.emplace_back(
          k_prot->enqueue(
              [k_l_p,k_s_p,this](){
                UploadDir(k_l_p,k_s_p);
              }
              )
          );
    } else{
      k_list.emplace_back(
          k_prot->enqueue(
              [k_l_p,k_s_p,this](){
                UploadFile(k_l_p,k_s_p);
              }
          )
      );
    }
  }

  std::future_status k_status{};
  auto k_it = k_list.begin();
  while (!k_list.empty()){
    k_status = k_it->wait_for(std::chrono::milliseconds{100});
    if(k_status == std::future_status::ready){
      try {
        k_it->get();
      } catch (const std::runtime_error& error) {
        DOODLE_LOG_DEBUG(error.what());
      }
      k_it = k_list.erase(k_it);
    } else{
      ++k_it;
    }
    if(k_it == k_list.end()){
      k_it = k_list.begin();
    }
  }
}

void RpcFileSystemClient::DownloadFile(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  auto k_is_down = compare_file_is_down(in_local_path, in_server_path);
  if (!k_is_down)
    return;

  if (!(*k_is_down))
    return;

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
}
void RpcFileSystemClient::UploadFile(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  auto k_is_down = compare_file_is_down(in_local_path, in_server_path);
  if (!k_is_down)
    return;

  if (*k_is_down)
    return;

  grpc::ClientContext k_context{};

  FileStream k_in_info{};
  FileInfo k_out_info{};

  FSys::ifstream k_file{in_local_path, std::ios::in | std::ios::binary};
  if (!k_file)
    throw DoodleError{"not read file"};

  k_in_info.mutable_info()->set_path(in_server_path.generic_string());
  auto k_in = p_stub->Upload(&k_context, &k_out_info);

  k_in->Write(k_in_info);
  auto s_size = CoreSet::getBlockSize();

  while (k_file) {
    std::string k_value{};
    k_value.resize(s_size);
    k_file.read(k_value.data(), s_size);
    auto k_s = k_file.gcount();
    if (k_s != s_size)
      k_value.resize(k_s);

    k_in_info.mutable_data()->set_value(std::move(k_value));
    if (!k_in->Write(k_in_info))
      throw DoodleError{"write stream errors"};
  }
}
}  // namespace doodle
