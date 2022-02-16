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
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/thread_pool/thread_pool.h>

#include <boost/numeric/conversion/cast.hpp>
#include <long_task/database_task.h>
#include <thread_pool/long_term.h>

#ifdef DOODLE_DIS_FUN
#include <grpcpp/grpcpp.h>
#endif
#include <doodle_lib/core/core_sig.h>
#include <core/status_info.h>
namespace doodle {

doodle_lib* doodle_lib::p_install = nullptr;

doodle_lib::doodle_lib()
    : p_thread_pool(new_object<thread_pool>(std::thread::hardware_concurrency() - 1)),
      p_log(new_object<logger_ctrl>()),
      reg(new_object<entt::registry>()) {
  /// 创建依赖性
  reg->on_construct<database>().connect<&database::set_enum>();
  reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();

  auto& k_sig = reg->set<core_sig>();
  reg->set<status_info>();
  k_sig.project_begin_open.connect([=](const FSys::path& in_path) {
    auto k_reg = g_reg();
    /// @brief 清除所有数据库实体
    auto k_v   = k_reg->view<database>();
    k_reg->destroy(k_v.begin(), k_v.end());
    /// @brief 清除所有孤立的实体
    k_reg->each([&k_reg](auto entity) {
      if (k_reg->orphan(entity)) {
        k_reg->release(entity);
      }
    });
  });
  k_sig.project_end_open.connect([](const entt::handle& in_handle, const doodle::project& in_project) {
    g_reg()->set<project>(in_project);
    g_reg()->set<database::ref_root>(in_handle.get<database>().get_ref());
    core_set::getSet().add_recent_project(in_project.get_path());
    g_reg()->set<root_ref>(in_handle);
    core_set_init{}.write_file();
  });
  k_sig.save_end.connect([](const std::vector<entt::handle>&) {
    g_reg()->ctx<status_info>().need_save = false;
  });
  p_install = this;
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
  date::set_install(k_local_path.generic_string());
  DOODLE_LOG_INFO("初始化时区数据库: {}", k_local_path.generic_string());
  return k_local_path;
}
doodle_lib& doodle_lib::Get() {
  return *p_install;
}

thread_pool_ptr doodle_lib::get_thread_pool() {
  return p_thread_pool;
}

doodle_lib::~doodle_lib() = default;

thread_pool& g_thread_pool() {
  return *(doodle_lib::Get().get_thread_pool());
};
}  // namespace doodle
