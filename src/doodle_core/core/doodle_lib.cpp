//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/logger/crash_reporting_thread.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/rules.h>

#include "core/doodle_lib.h"

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include <core/status_info.h>
#include <exception/exception.h>
#include <logger/logger.h>
#include <memory>
#include <utility>
#include <wil/result.h>

namespace doodle {

class doodle_lib::impl {
 public:
  boost::asio::io_context io_context_{};
  boost::asio::thread_pool thread_pool_attr{std::thread::hardware_concurrency() * 2};
  logger_ctr_ptr p_log{};
  registry_ptr reg{};
  entt::registry::context ctx_p{std::allocator<entt::entity>{}};

  inline static doodle_lib* self;
};

doodle_lib::doodle_lib() : ptr() {
  impl::self = this;
  ptr        = std::move(std::make_unique<impl>());
  init();
}

doodle_lib& doodle_lib::Get() { return *impl::self; }
void doodle_lib::init() {
  /// @brief 初始化其他
  ptr->reg   = std::make_shared<entt::registry>();
  ptr->p_log = std::make_shared<logger_ctr_ptr::element_type>();
  wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept {
    constexpr std::size_t sizeOfLogMessageWithNul = 2048;

    wchar_t logMessage[sizeOfLogMessageWithNul];
    if (SUCCEEDED(wil::GetFailureLogString(logMessage, sizeOfLogMessageWithNul, failure))) {
      default_logger_raw()->log(level::warn, boost::locale::conv::utf_to_utf<char>(logMessage));
    }
  });
  // ptr->ctx_p.emplace<detail::crash_reporting_thread>();
  ptr->ctx_p.emplace<database_info>();
  ptr->ctx_p.emplace<database_n::file_translator_ptr>(std::make_shared<database_n::file_translator>(ptr->reg));
  ptr->ctx_p.emplace<status_info>();

  // boost::locale::generator k_gen{};
  // k_gen.categories(
  //     boost::locale::all_categories ^ boost::locale::category_t::formatting ^ boost::locale::category_t::parsing
  //);
  // FSys::path::imbue(k_gen("zh_CN.UTF-8"));

  ptr->reg->ctx().emplace<project>("C:/", "tmp_project");
  ptr->reg->ctx().emplace<project_config::base_config>(project_config::base_config::get_default());
  ptr->reg->ctx().emplace<user::current_user>();
  ptr->ctx_p.emplace<core_sig>();
}

bool doodle_lib::operator==(const doodle_lib& in_rhs) const { return ptr == in_rhs.ptr; }

doodle_lib::~doodle_lib() = default;

registry_ptr& g_reg() { return doodle_lib::Get().ptr->reg; }
boost::asio::io_context& g_io_context() { return doodle_lib::Get().ptr->io_context_; }
boost::asio::thread_pool& g_thread() { return doodle_lib::Get().ptr->thread_pool_attr; }
details::logger_ctrl& g_logger_ctrl() { return *doodle_lib::Get().ptr->p_log; }
entt::registry::context& g_ctx() { return doodle_lib::Get().ptr->ctx_p; }
details::database_info& g_db() {
  static details::database_info db;
  return db;
}

details::database_pool_info& g_pool_db() {
  static details::database_pool_info db{};
  return db;
}

boost::asio::strand<boost::asio::io_context::executor_type>& g_strand() {
  static auto strand = boost::asio::make_strand(g_io_context());
  return strand;
}
}  // namespace doodle
