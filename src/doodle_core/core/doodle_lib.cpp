//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <date/tz.h>
#include <logger/logger.h>
#include <doodle_core/core/core_set.h>
#include <exception/exception.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/rules.h>
#include <thread_pool/thread_pool.h>

#include <doodle_core/core/core_sig.h>
#include <core/status_info.h>

#include <boost/locale.hpp>

#include <doodle_core/database_task/sqlite_client.h>
namespace doodle {

doodle_lib* doodle_lib::p_install = nullptr;

class doodle_lib::impl {
 public:
  std::shared_ptr<boost::asio::io_context> io_context_{std::make_shared<boost::asio::io_context>()};
  thread_pool_ptr p_thread_pool{std::make_shared<thread_pool>(std::thread::hardware_concurrency() * 2)};
  logger_ctr_ptr p_log{std::make_shared<logger_ctrl>()};
  registry_ptr reg{std::make_shared<entt::registry>()};

  scheduler_t loop{};
  bounded_pool_t loop_bounded_pool{};
};

doodle_lib::doodle_lib()
    : ptr(std::make_unique<impl>()) {
  boost::locale::generator k_gen{};
  k_gen.categories(boost::locale::all_categories ^ boost::locale::formatting_facet ^ boost::locale::parsing_facet);
  FSys::path::imbue(k_gen("zh_CN.UTF-8"));
  ptr->loop_bounded_pool.timiter_ = core_set::getSet().p_max_thread;
  /// 创建依赖性
  ptr->reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();

  ptr->reg->ctx().emplace<database_info>();
  ptr->reg->ctx().emplace<project>("C:/", "tmp_project");
  ptr->reg->ctx().emplace<project_config::base_config>();
  ptr->reg->ctx().emplace<user>("user"s);

  auto& k_sig = ptr->reg->ctx().emplace<core_sig>();
  ptr->reg->ctx().emplace<status_info>();
  k_sig.save.connect(2, []() {
    std::make_shared<database_n::sqlite_file>()
        ->async_save(g_reg()->ctx().at<::doodle::database_info>().path_, [](auto) {
          DOODLE_LOG_INFO("保存项目 {}", g_reg()->ctx().at<::doodle::database_info>().path_);
        });
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

      DOODLE_CHICK(k_ofstream, doodle_error{"无法创建数据库 {}", k_local_path / i.filename()});
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

thread_pool_ptr doodle_lib::get_thread_pool() const {
  return ptr->p_thread_pool;
}
registry_ptr& doodle_lib::reg_attr() const {
  return ptr->reg;
}
scheduler_t& doodle_lib::main_loop_attr() const {
  return ptr->loop;
}
bounded_pool_t& doodle_lib::bounded_pool_attr() const {
  return ptr->loop_bounded_pool;
}
boost::asio::io_context& doodle_lib::io_context_attr() const {
  return *ptr->io_context_;
}
boost::asio::thread_pool& doodle_lib::thread_attr() const {
  return ptr->p_thread_pool->pool_;
}
bool doodle_lib::operator==(const doodle_lib& in_rhs) const {
  return ptr == in_rhs.ptr;
}

doodle_lib::~doodle_lib() = default;

thread_pool& g_thread_pool() {
  return *(doodle_lib::Get().get_thread_pool());
};

registry_ptr& g_reg() {
  return doodle_lib::Get().reg_attr();
}
scheduler_t& g_main_loop() {
  return doodle_lib::Get().main_loop_attr();
}
bounded_pool_t& g_bounded_pool() {
  return doodle_lib::Get().bounded_pool_attr();
}
boost::asio::io_context& g_io_context() {
  return doodle_lib::Get().io_context_attr();
}
boost::asio::thread_pool& g_thread() {
  return doodle_lib::Get().thread_attr();
}

}  // namespace doodle
