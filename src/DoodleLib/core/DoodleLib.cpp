//
// Created by TD on 2021/6/17.
//

#include "DoodleLib.h"

#include <Exception/Exception.h>
#include <Logger/Logger.h>
#include <Metadata/MetadataFactory.h>
#include <core/CoreSet.h>
#include <date/tz.h>
#include <grpcpp/grpcpp.h>
#include <rpc/RpcFileSystemClient.h>
#include <rpc/RpcMetadataClient.h>
#include <threadPool/ThreadPool.h>
namespace doodle {

DoodleLib* DoodleLib::p_install = nullptr;

DoodleLib::DoodleLib()
    : p_thread_pool(std::make_shared<ThreadPool>(4)),
      p_curr_project(),
      p_rpc_metadata_clien(),
      p_rpc_file_system_client(),
      p_metadata_factory() {
  CoreSet::getSet();
  Logger::doodle_initLog();
  /// 在这里我们初始化date tz 时区数据库
  auto k_path = create_time_database();
  date::set_install(k_path.generic_string());
  DOODLE_LOG_INFO("初始化时区数据库: {}", k_path.generic_string());
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
        throw DoodleError{fmt::format("无法创建数据库 {}", k_local_path / i.filename())};
      }
    }
  }
  return k_local_path;
}
DoodleLib& DoodleLib::Get() {
  return *p_install;
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

  Logger::clear();
}
const ProjectPtr& DoodleLib::current_project() const {
  return p_curr_project;
}
void DoodleLib::set_current_project(const ProjectPtr& in_currProject) {
  auto it = std::find_if(
      p_project_vector.begin(), p_project_vector.end(),
      [in_currProject](const ProjectPtr& in_ptr) { return in_ptr->getId() == in_currProject->getId(); });
  if (it == p_project_vector.end()) {
    DOODLE_LOG_WARN("无法找到项目: {}", in_currProject->str());
    throw DoodleError{fmt::format("无法找到项目: {}", in_currProject->str())};
  }
  p_curr_project = in_currProject;
}
void DoodleLib::init_gui() {
  auto k_ip = fmt::format("{}:{:d}", CoreSet::getSet().get_server_host(), CoreSet::getSet().getMetaRpcPort());

  DOODLE_LOG_DEBUG(k_ip)

  p_rpc_metadata_clien = std::make_shared<RpcMetadataClient>(
      grpc::CreateChannel(k_ip,
                          grpc::InsecureChannelCredentials()));

  k_ip = fmt::format("{}:{:d}", CoreSet::getSet().get_server_host(), CoreSet::getSet().getFileRpcPort());
  DOODLE_LOG_DEBUG(k_ip)
  p_rpc_file_system_client = std::make_shared<RpcFileSystemClient>(
      grpc::CreateChannel(k_ip,
                          grpc::InsecureChannelCredentials()));

  p_project_vector = std::make_shared<MetadataFactory>()->getAllProject();
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
}  // namespace doodle
