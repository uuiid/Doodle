//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <date/tz.h>
#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/rpc/rpc_file_system_client.h>
#include <doodle_lib/rpc/rpc_metadata_client.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <grpcpp/grpcpp.h>

#include <boost/numeric/conversion/cast.hpp>
namespace doodle {

doodle_lib* doodle_lib::p_install = nullptr;

doodle_lib::doodle_lib()
    : p_thread_pool(new_object<thread_pool>(core_set::getSet().p_max_thread)),
      p_log(new_object<logger_ctrl>()),
      p_rpc_metadata_clien(),
      p_rpc_file_system_client(),
      p_metadata_factory(),
      long_task_list(),
      mutex(),
      reg(new_object<entt::registry>()) {
#ifdef _WIN32
  /// 在这里我们初始化date tz 时区数据库
  auto k_path = create_time_database();
  date::set_install(k_path.generic_string());
  DOODLE_LOG_INFO("初始化时区数据库: {}", k_path.generic_string());
#endif

  /// 创建依赖性
  reg->on_construct<project>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<project>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<project>().connect<&database::set_enum>();

  reg->on_construct<season>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<season>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<season>().connect<&database::set_enum>();

  reg->on_construct<episodes>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<episodes>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<episodes>().connect<&database::set_enum>();

  reg->on_construct<shot>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<shot>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<shot>().connect<&database::set_enum>();

  reg->on_construct<assets>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<assets>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<assets>().connect<&database::set_enum>();

  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<database_root>>();
  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();
  reg->on_construct<assets_file>().connect<&database::set_enum>();

  reg->on_construct<database>().connect<&entt::registry::get_or_emplace<database_stauts>>();
}

FSys::path doodle_lib::create_time_database() {
  auto k_local_path = core_set::getSet().get_cache_root("tzdata");
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
doodle_lib& doodle_lib::Get() {
  return *p_install;
}

void doodle_lib::post_constructor() {
  p_install = this;
}

void doodle_lib::set_thread_pool_size() {
  p_thread_pool = new_object<thread_pool>(core_set::getSet().p_max_thread);
}
thread_pool_ptr doodle_lib::get_thread_pool() {
  return p_thread_pool;
}

doodle_lib::~doodle_lib() {
  p_project_vector.clear();
}
void doodle_lib::init_gui() {
  p_thread_pool = new_object<thread_pool>(core_set::getSet().p_max_thread);
  auto k_ip = fmt::format("{}:{:d}", core_set::getSet().get_server_host(), core_set::getSet().get_meta_rpc_port());

  DOODLE_LOG_DEBUG(k_ip);

  try {
    p_rpc_metadata_clien = new_object<rpc_metadata_client>(
        grpc::CreateChannel(k_ip,
                            grpc::InsecureChannelCredentials()));

    k_ip = fmt::format("{}:{:d}", core_set::getSet().get_server_host(), core_set::getSet().get_file_rpc_port());
    DOODLE_LOG_DEBUG(k_ip)
    p_rpc_file_system_client = new_object<rpc_file_system_client>(
        grpc::CreateChannel(k_ip,
                            grpc::InsecureChannelCredentials()));

    p_metadata_factory = new_object<metadata_serialize>();
    p_project_vector   = p_metadata_factory->get_all_prj();
  } catch (doodle_error& err) {
    p_rpc_file_system_client.reset();
    p_rpc_metadata_clien.reset();
    DOODLE_LOG_ERROR(err.what());
  }
}
rpc_metadata_client_ptr doodle_lib::get_rpc_metadata_client() const {
  return p_rpc_metadata_clien;
}
rpc_file_system_client_ptr doodle_lib::get_rpc_file_system_client() const {
  return p_rpc_file_system_client;
}

metadata_serialize_ptr doodle_lib::get_metadata_factory() const {
  return p_metadata_factory;
}
}  // namespace doodle
