//
// Created by TD on 2022/5/30.
//

#include "app_base.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/rules.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>
#include <boost/system/detail/error_code.hpp>

#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <processthreadsapi.h>
#include <processtopologyapi.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <wil/result.h>
#include <wil/result_macros.h>
#include <winnt.h>

namespace doodle {

app_base* app_base::self = nullptr;
bool app_base::init() { return true; }
app_base::app_base(int argc, const char* const argv[]) : arg_{argc, argv} {
  self = this;
  init_lib();
  default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
  default_logger_raw()->flush();
  add_signal();
}
void app_base::set_exit_code(std::int32_t in_exit_code) {
  if (exit_code == 0) exit_code = in_exit_code;
}

namespace {
struct comline_data {
  explicit comline_data(std::int32_t argc, const wchar_t* const argv[]) {
    for (std::int32_t i = 0; i < argc; ++i) {
      arg_data.emplace_back(boost::locale::conv::utf_to_utf<char>(argv[i]));
    }
    p_buff = std::make_shared<char*[]>(argc);
    for (std::int32_t i = 0; i < argc; ++i) {
      p_buff[i] = arg_data[i].data();
    }
    argv_ = p_buff.get();
  }
  std::vector<std::string> arg_data;
  std::shared_ptr<char*[]> p_buff;
  const char* const* argv_;
};

}  // namespace

app_base::app_base(std::int32_t argc, const wchar_t* const argv[]) : app_base(argc, comline_data{argc, argv}.argv_) {}

void app_base::init_lib() {
  SPDLOG_WARN("Doodle Lib 初始化完成 使用处理器 {}", get_hardware_concurrency());
  auto l_max = 15;
  for (std::size_t i = 0; i < l_max; ++i) {
    strands_.emplace_back(boost::asio::make_strand(io_context_));
  }
  wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept {
    constexpr std::size_t sizeOfLogMessageWithNul = 2048;

    wchar_t logMessage[sizeOfLogMessageWithNul];
    if (SUCCEEDED(wil::GetFailureLogString(logMessage, sizeOfLogMessageWithNul, failure))) {
      default_logger_raw()->log(level::warn, boost::locale::conv::utf_to_utf<char>(logMessage));
    }
  });
}

void app_base::clear_global_state() {
  core_set::get_set().computers_assign_task_ptr_.reset();
  core_set::get_set().database_.reset();
  core_set::get_set().ctx_ptr.reset();
  core_set::get_set().seed_email_ = email::seed_email{"", 0, "", ""};
  core_set::get_set().ort_env_ptr_.reset();
}

bool app_base::operator==(const app_base& in_rhs) const { return this == &in_rhs; }

boost::asio::io_context& g_io_context() { return app_base::Get().io_context_; }
details::logger_ctrl& g_logger_ctrl() { return *app_base::Get().p_log; }

http::kitsu_ctx_t& g_kitsu_ctx() {
  auto l_ptr = core_set::get_set().kitsu_ctx_;
  if (!l_ptr) throw_exception(doodle_error{"kitsu_ctx_t 未初始化"});
  return *l_ptr;
}
boost::asio::strand<boost::asio::io_context::executor_type>& g_strand() { return app_base::Get().strand_; }
boost::asio::strand<boost::asio::io_context::executor_type>& g_pool_strand() {
  return app_base::Get().strands_[app_base::Get().strand_index_++ % app_base::Get().strands_.size()];
}
std::size_t get_hardware_concurrency() {
  return std::max(
      boost::numeric_cast<std::size_t>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)),
      boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency())
  );
}
sqlite_database get_sqlite_database() {
  auto& db = core_set::get_set().database_;
  DOODLE_CHICK(db, "错误: 数据库未打开")
  return sqlite_database{db->get_strand(), db->create_session()};
}
Ort::Env& get_ort_env() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() {
    try {
      auto env                         = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "doodle_ort");
      core_set::get_set().ort_env_ptr_ = env;
      SPDLOG_INFO("ONNX Runtime 环境初始化成功");
    } catch (const Ort::Exception& e) {
      SPDLOG_ERROR("ONNX Runtime 环境初始化失败: {}", e.what());
    }
  });
  auto& env = core_set::get_set().ort_env_ptr_;
  DOODLE_CHICK(env, "错误: ONNX Runtime 环境未初始化")
  return *std::static_pointer_cast<Ort::Env>(env);
}

app_base::~app_base() {
  // sig_ptr 上挂着一个 async_wait, 回调捕获了 this。析构顺序是: sig_ptr 在 io_context_ 之后
  // 声明 => 先析构, 而 io_context_ 要等 app_base 析构才拆。若这里不管,
  // 这个待处理的 signal_set 会一直活到 io_context_ 析构时才被销毁(取消), 那时 this 已经
  // 不在了 —— 表现为析构阶段的访问违例(address 0xffffffffffffffff), 单测(如
  // http_listener_port_shm)一构造 app_base 就必崩。
  // 这里先显式取消并拆掉, 让取消回调在本对象仍存活时跑完。
  if (sig_ptr) {
    boost::system::error_code l_ec{};
    sig_ptr->cancel(l_ec);
    sig_ptr.reset();
  }
  io_context_.stop();
  while (io_context_.poll_one() != 0) {
  }
  clear_global_state();
}

app_base& app_base::Get() { return *self; }
app_base* app_base::GetPtr() { return self; }

void app_base::bind_thread_to_group(int group_id) {
  // 获取组中处理器数量
  auto l_a = GetActiveProcessorCount(group_id);

  wil::unique_handle process_handle{GetCurrentProcess()};
  GROUP_AFFINITY group_affinity{};
  group_affinity.Group = static_cast<WORD>(group_id);
  // 设置掩码, 选择组内的所有处理器
  for (decltype(l_a) i = 0; i < l_a; ++i) {
    group_affinity.Mask |= (1ULL << i);
  }
  LOG_IF_WIN32_BOOL_FALSE(::SetThreadGroupAffinity(GetCurrentThread(), &group_affinity, nullptr));
}

std::int32_t app_base::run() {
  stop_ = !init();
  if (stop_) return 0;
  auto l_logger = g_logger_ctrl().get_main_error();
  if (!use_multithread_) {
    try {
      g_io_context().run();
    } catch (const doodle_error& in_err) {
      set_exit_code(in_err.error_code_ == 0 ? -1 : in_err.error_code_);
      l_logger->error(in_err.what());
    } catch (const std::system_error& in_err) {
      set_exit_code(in_err.code().value());
      l_logger->error(in_err.what());
    } catch (...) {
      set_exit_code(1);
      l_logger->error(boost::current_exception_diagnostic_information());
    }
  } else {
    std::vector<std::thread> l_threads{};
    auto l_thread_count = get_hardware_concurrency() == 0 ? 8 : get_hardware_concurrency() - 1;
    l_threads.reserve(l_thread_count);
    for (std::size_t i = 0; i < l_thread_count; ++i) {
      l_threads.emplace_back([this, i, l_logger]() {
        bind_thread_to_group(static_cast<int>(i % static_cast<std::size_t>(GetActiveProcessorGroupCount())));
        for (;;) try {
            g_io_context().run();
            break;
          } catch (...) {
            l_logger->error(boost::current_exception_diagnostic_information());
          }
      });
    }
    for (auto& l_thread : l_threads) {
      l_thread.join();
    }
  }
  auto& io = g_io_context();
  io.stop();
  while (io.poll_one() != 0) {
  }

  facets_.clear();
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& in_ptr) { in_ptr->flush(); });
  return exit_code;
}

void app_base::stop_app(std::int32_t in_exit_code) {
  bool l_expected = false;
  if (!stop_.compare_exchange_strong(l_expected, true)) return;

  exit_code = in_exit_code;
  on_cancel.emit();
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& in_ptr) { in_ptr->flush(); });
  g_io_context().stop();
  while (g_io_context().poll_one() != 0) {
  }
}

bool app_base::is_main_thread() const { return run_id == std::this_thread::get_id(); }
void app_base::use_multithread(bool in_use) { use_multithread_ = in_use; }

void app_base::add_signal() {
  sig_ptr = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  sig_ptr->async_wait([this](const boost::system::error_code& ec, int signal_number) {
    if (ec) return;
    stop_app(0);
  });
}
}  // namespace doodle
