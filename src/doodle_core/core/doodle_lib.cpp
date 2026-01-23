//
// Created by TD on 2021/6/17.
//

#include "doodle_lib.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/logger/crash_reporting_thread.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/rules.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include <logger/logger.h>
#include <memory>
#include <utility>
#include <wil/result.h>

namespace doodle {

class doodle_lib::impl {
 public:
  boost::asio::io_context io_context_{};
  entt::registry::context ctx_p{std::allocator<entt::entity>{}};
  using strand_type = boost::asio::strand<boost::asio::io_context::executor_type>;
  strand_type strand_{boost::asio::make_strand(io_context_)};
  std::vector<strand_type> strands_{};
  std::atomic_size_t strand_index_{0};
  logger_ctr_ptr p_log{std::make_shared<logger_ctr_ptr::element_type>()};
  inline static doodle_lib* self;
};

doodle_lib::doodle_lib() : ptr() {
  impl::self = this;
  ptr        = std::move(std::make_unique<impl>());
  SPDLOG_WARN("Doodle Lib 初始化完成 使用处理器 {}", get_hardware_concurrency());
  // 初始化多个 strand 用于线程池
  auto l_max = 10;
  for (std::size_t i = 0; i < l_max; ++i) {
    ptr->strands_.emplace_back(boost::asio::make_strand(ptr->io_context_));
  }

  init();
}

doodle_lib& doodle_lib::Get() { return *impl::self; }
void doodle_lib::init() {
  /// @brief 初始化其他
  wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept {
    constexpr std::size_t sizeOfLogMessageWithNul = 2048;

    wchar_t logMessage[sizeOfLogMessageWithNul];
    if (SUCCEEDED(wil::GetFailureLogString(logMessage, sizeOfLogMessageWithNul, failure))) {
      default_logger_raw()->log(level::warn, boost::locale::conv::utf_to_utf<char>(logMessage));
    }
  });
}

bool doodle_lib::operator==(const doodle_lib& in_rhs) const { return ptr == in_rhs.ptr; }

doodle_lib::~doodle_lib() = default;

boost::asio::io_context& g_io_context() { return doodle_lib::Get().ptr->io_context_; }
details::logger_ctrl& g_logger_ctrl() { return *doodle_lib::Get().ptr->p_log; }
entt::registry::context& g_ctx() { return doodle_lib::Get().ptr->ctx_p; }
boost::asio::strand<boost::asio::io_context::executor_type>& g_strand() { return doodle_lib::Get().ptr->strand_; }
boost::asio::strand<boost::asio::io_context::executor_type>& g_pool_strand() {
  return doodle_lib::Get()
      .ptr->strands_[doodle_lib::Get().ptr->strand_index_++ % doodle_lib::Get().ptr->strands_.size()];
}
std::size_t get_hardware_concurrency() {
  return std::max(
      boost::numeric_cast<std::size_t>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)),
      boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency())
  );
}

}  // namespace doodle
