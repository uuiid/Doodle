//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <date/tz.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/thread_pool/thread_pool.h>

#include <boost/numeric/conversion/cast.hpp>
#include <long_task/database_task.h>
#include <thread_pool/long_term.h>

#ifdef DOODLE_GRPC
#include <grpcpp/grpcpp.h>
#endif

namespace doodle {

doodle_lib* doodle_lib::p_install = nullptr;

doodle_lib::doodle_lib()
    : p_thread_pool(new_object<thread_pool>(core_set::getSet().p_max_thread)),
      p_log(new_object<logger_ctrl>()),
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
  reg->on_construct<project>().connect<&entt::registry::get_or_emplace<root_ref>>();
  reg->on_construct<project>().connect<&database::set_enum>();

  reg->on_construct<season>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<season>().connect<&entt::registry::get_or_emplace<root_ref>>();

  reg->on_construct<episodes>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<episodes>().connect<&entt::registry::get_or_emplace<root_ref>>();

  reg->on_construct<shot>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<shot>().connect<&entt::registry::get_or_emplace<root_ref>>();

  reg->on_construct<assets>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<assets>().connect<&entt::registry::get_or_emplace<root_ref>>();

  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<database>>();
  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<root_ref>>();
  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();

  reg->on_construct<database>().connect<&entt::registry::get_or_emplace<database_stauts>>();
  p_install = this;
  core_sql::Get();
}

FSys::path doodle_lib::create_time_database() {
  auto k_local_path = core_set::getSet().get_cache_root("tzdata");
  if (FSys::is_empty(k_local_path)) {
    auto k_path = cmrc::DoodleLibResource::get_filesystem().iterate_directory("resource/tzdata");
    for (const auto& i : k_path) {
      FSys::ofstream k_ofstream{k_local_path / i.filename(), std::ios::out | std::ios::binary};
      DOODLE_LOG_INFO("开始创建数据库 {}", k_local_path / i.filename());

      chick_true<doodle_error>(k_ofstream, DOODLE_LOC, "无法创建数据库 {}", k_local_path / i.filename());
      auto k_file = cmrc::DoodleLibResource::get_filesystem().open("resource/tzdata/" + i.filename());
      k_ofstream.write(k_file.begin(), boost::numeric_cast<std::int64_t>(k_file.size()));
    }
  }
  return k_local_path;
}
doodle_lib& doodle_lib::Get() {
  return *p_install;
}

thread_pool_ptr doodle_lib::get_thread_pool() {
  return p_thread_pool;
}

doodle_lib::~doodle_lib() = default;
void doodle_lib::init_gui() {
  p_thread_pool = new_object<thread_pool>(std::thread::hardware_concurrency() - 1);
  loop_bounded_pool.set_bounded(boost::numeric_cast<std::uint16_t>(core_set::getSet().p_max_thread));

  //  try {
  //    //    auto k_h = make_handle();
  //    //    k_h.emplace<process_message>();
  //    //    auto k_then = g_main_loop().attach<null_process_t>();
  //    //    for (auto& l_p : core_set::getSet().project_root) {
  //    //      k_then.then<database_task_select>(k_h, l_p);
  //    //    }
  //    //    k_h.destroy();
  //    //    k_then.then([]() {
  //    //      core_set_init{}.init_default_project();
  //    //    });
  //  } catch (doodle_error& err) {
  //    DOODLE_LOG_ERROR(err.what());
  //  }
}

thread_pool& g_thread_pool() {
  return *(doodle_lib::Get().get_thread_pool());
};
}  // namespace doodle
