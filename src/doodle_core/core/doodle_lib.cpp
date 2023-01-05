//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/rules.h>

#include "core/doodle_lib.h"

#include <boost/locale.hpp>

#include <core/status_info.h>
#include <date/tz.h>
#include <exception/exception.h>
#include <logger/logger.h>
#include <memory>
#include <utility>

namespace doodle {

class doodle_lib::impl {
 public:
  std::shared_ptr<boost::asio::io_context> io_context_{};
  boost::asio::thread_pool thread_pool_attr{std::thread::hardware_concurrency() * 2};
  logger_ctr_ptr p_log{};
  registry_ptr reg{};

  inline static doodle_lib* self;
};

doodle_lib::doodle_lib() : ptr() {
  impl::self = this;
  init();
}

doodle_lib& doodle_lib::Get() { return *impl::self; }
void doodle_lib::init() {
  ptr              = std::move(std::make_unique<impl>());
  /// @brief 初始化其他
  ptr->io_context_ = std::make_shared<boost::asio::io_context>();
  ptr->p_log       = std::make_shared<logger_ctrl>();
  ptr->reg         = std::make_shared<entt::registry>();

  boost::locale::generator k_gen{};
  k_gen.categories(boost::locale::all_categories ^ boost::locale::formatting_facet ^ boost::locale::parsing_facet);
  FSys::path::imbue(k_gen("zh_CN.UTF-8"));
  /// 创建依赖性
  ptr->reg->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();

  ptr->reg->ctx().emplace<database_info>().path_ = ":memory:"s;
  ptr->reg->ctx().emplace<project>("C:/", "tmp_project");
  ptr->reg->ctx().emplace<project_config::base_config>();
  ptr->reg->ctx().emplace<user::current_user>();
  ptr->reg->ctx().emplace<program_info>();

  ptr->reg->ctx().emplace<status_info>();
  core_set::get_set().lib_ptr = this;
  ptr->reg->ctx().emplace<database_n::file_translator_ptr>(std::make_shared<database_n::sqlite_file>());
}

registry_ptr& doodle_lib::reg_attr() const { return ptr->reg; }

boost::asio::io_context& doodle_lib::io_context_attr() const { return *ptr->io_context_; }

bool doodle_lib::operator==(const doodle_lib& in_rhs) const { return ptr == in_rhs.ptr; }

doodle_lib::~doodle_lib() = default;

boost::asio::thread_pool& doodle_lib::thread_attr() const { return ptr->thread_pool_attr; }

registry_ptr& g_reg() { return doodle_lib::Get().reg_attr(); }

boost::asio::io_context& g_io_context() { return doodle_lib::Get().io_context_attr(); }
boost::asio::thread_pool& g_thread() { return doodle_lib::Get().thread_attr(); }

}  // namespace doodle
