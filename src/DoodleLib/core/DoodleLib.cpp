//
// Created by TD on 2021/6/17.
//

#include "DoodleLib.h"

#include <Exception/exception.h>
#include <Logger/logger.h>
#include <Metadata/metadata_factory.h>
#include <core/CoreSet.h>
#include <date/tz.h>
#include <grpcpp/grpcpp.h>
#include <rpc/RpcFileSystemClient.h>
#include <rpc/RpcMetadataClient.h>
#include <threadPool/thread_pool.h>

#include <boost/numeric/conversion/cast.hpp>
namespace doodle {

DoodleLib* DoodleLib::p_install = nullptr;

DoodleLib::DoodleLib()
    : p_thread_pool(new_object<thread_pool>(CoreSet::getSet().p_max_thread)),
      p_curr_project(),
      p_rpc_metadata_clien(),
      p_rpc_file_system_client(),
      p_metadata_factory(),
      long_task_list(),
      mutex() {
  CoreSet::getSet();
  logger::doodle_initLog();
#ifdef _WIN32
  /// 在这里我们初始化date tz 时区数据库
  auto k_path = create_time_database();
  date::set_install(k_path.generic_string());
  DOODLE_LOG_INFO("初始化时区数据库: {}", k_path.generic_string());
#endif
}

FSys::path DoodleLib::create_time_database() {
  auto k_local_path = CoreSet::getSet().getCacheRoot("tzdata");
  if (FSys::is_empty(k_local_path)) {
    auto k_path = cmrc::DoodleLibResource::get_filesystem().iterate_directory("resource/tzdata");
    for (const auto& i : k_path) {
      FSys::ofstream k_ofstream{k_local_path / i.filename(), std::ios::out | std::ios::binary};
      DOODLE_LOG_INFO("开始创建数据库 {}", k_local_path / i.filename());
      if (k_ofstream) {
        auto k_file = cmrc::DoodleLibResource::get_filesystem().open("resource/tzdata/" + i.filename());
        k_ofstream.write(k_file.begin(), boost::numeric_cast<std::int64_t>(k_file.size()));
      } else {
        DOODLE_LOG_INFO("无法创建数据库 {}", k_local_path / i.filename());
        throw doodle_error{fmt::format("无法创建数据库 {}", k_local_path / i.filename())};
      }
    }
  }
  return k_local_path;
}
DoodleLib& DoodleLib::Get() {
  return *p_install;
}

void DoodleLib::set_thread_pool_size() {
  p_thread_pool = new_object<thread_pool>(CoreSet::getSet().p_max_thread);
}
ThreadPoolPtr DoodleLib::get_thread_pool() {
  return p_thread_pool;
}
[[maybe_unused]] DoodleLibPtr make_doodle_lib() {
  auto ptr             = std::unique_ptr<DoodleLib>(new DoodleLib{});
  DoodleLib::p_install = ptr.get();
  return ptr;
}
DoodleLib::~DoodleLib() {
  p_project_vector.clear();
  p_curr_project.reset();

  logger::clear();
}
void DoodleLib::init_gui() {
  auto k_ip = fmt::format("{}:{:d}", CoreSet::getSet().get_server_host(), CoreSet::getSet().getMetaRpcPort());

  DOODLE_LOG_DEBUG(k_ip)

  p_rpc_metadata_clien = new_object<RpcMetadataClient>(
      grpc::CreateChannel(k_ip,
                          grpc::InsecureChannelCredentials()));

  k_ip = fmt::format("{}:{:d}", CoreSet::getSet().get_server_host(), CoreSet::getSet().getFileRpcPort());
  DOODLE_LOG_DEBUG(k_ip)
  p_rpc_file_system_client = new_object<RpcFileSystemClient>(
      grpc::CreateChannel(k_ip,
                          grpc::InsecureChannelCredentials()));

  p_metadata_factory = new_object<metadata_factory>();
  p_project_vector   = p_metadata_factory->getAllProject();
  if (!p_project_vector.empty())
    if (p_curr_project) {
      auto it = std::find_if(p_project_vector.begin(), p_project_vector.end(),
                             [this](const ProjectPtr& in_ptr) { return in_ptr->getId() == this->p_curr_project->getId(); });
      if (it != p_project_vector.end())
        p_curr_project = *it;
      else
        p_curr_project = p_project_vector.front();
    } else
      p_curr_project = p_project_vector.front();
}
RpcMetadataClientPtr DoodleLib::getRpcMetadataClient() const {
  return p_rpc_metadata_clien;
}
RpcFileSystemClientPtr DoodleLib::getRpcFileSystemClient() const {
  return p_rpc_file_system_client;
}

MetadataFactoryPtr DoodleLib::get_metadata_factory() const {
  return p_metadata_factory;
}
}  // namespace doodle
