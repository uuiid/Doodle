#include "process_message.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>
#include <doodle_core/logger/logger.h>

#include <magic_enum.hpp>
#include <range/v3/all.hpp>
#include <range/v3/range.hpp>
#include <spdlog/spdlog.h>

namespace doodle {

namespace details {
template <class Mutex>
class process_message_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<process_message::data_t> data_;

 public:
  explicit process_message_sink(std::shared_ptr<process_message::data_t> in_process_message)
      : data_(std::move(in_process_message)) {}

 private:
 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    std::lock_guard const _lock{data_->_mutex};
    if (data_->info_.empty() && data_->warn_.empty()) {
      data_->p_state = process_message::state::run;
      data_->p_time  = chrono::system_clock::now();
    }
    constexpr auto g_success = magic_enum::enum_name(process_message::state::success);
    constexpr auto g_fail    = magic_enum::enum_name(process_message::state::fail);
    switch (msg.level) {
      case spdlog::level::level_enum::trace:
        data_->trace_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 10000};
        break;
      case spdlog::level::level_enum::debug:
        data_->debug_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 1000};
        break;
      case spdlog::level::level_enum::info:
        data_->info_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 1000};
        break;
      case spdlog::level::level_enum::warn:
        data_->warn_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 100};
        break;
      case spdlog::level::level_enum::err:
        data_->err_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 10};
        break;
      case spdlog::level::level_enum::critical:
        data_->critical_.append(formatted.data(), formatted.size());
        data_->p_progress += {1, 10};
        break;
      case spdlog::level::level_enum::off:
        data_->p_end      = chrono::system_clock::now();
        data_->p_progress = {1, 1};
        if (msg.payload == g_success) {
          data_->p_state = process_message::state::success;
        } else if (msg.payload == g_fail) {
          data_->p_state = process_message::state::fail;
        }
        break;

      case spdlog::level::level_enum::n_levels:
        break;
    }
    switch (msg.level) {
      case spdlog::level::level_enum::trace:
      case spdlog::level::level_enum::debug:
      case spdlog::level::level_enum::info:
        break;
      case spdlog::level::level_enum::warn:
      case spdlog::level::level_enum::err:
      case spdlog::level::level_enum::critical:
        data_->p_str_end = fmt::to_string(formatted);
        break;
      case spdlog::level::level_enum::off:
      case spdlog::level::level_enum::n_levels:
        break;
    }

    if (data_->p_progress > 1) --data_->p_progress;
  }
  void flush_() override {}
};
}  // namespace details
using process_message_sink_mt = details::process_message_sink<std::mutex>;

process_message::process_message(std::string in_name) : data_(std::make_shared<data_t>()) {
  data_->p_state   = state::wait;
  data_->p_time    = chrono::system_clock::now();

  data_->p_name    = std::move(in_name);
  data_->p_name_id = fmt::format("{}##{}", data_->p_name, fmt::ptr(this));

  data_->p_logger  = g_logger_ctrl().make_log(data_->p_name);
  data_->p_logger->sinks().emplace_back(std::make_shared<process_message_sink_mt>(this->data_));
}

const std::string& process_message::get_name() const { return data_->p_name; }
logger_ptr process_message::logger() const { return data_->p_logger; }

void process_message::progress_step(const rational_int& in_rational_int) {
  std::lock_guard _lock{data_->_mutex};
  data_->p_progress += in_rational_int;
  if (data_->p_progress > 1) --data_->p_progress;
}

void process_message::set_state(state in_state) {
  std::lock_guard _lock{data_->_mutex};
  switch (in_state) {
    case run:
      data_->p_time = chrono::system_clock::now();
    case wait:
      break;
    case success:
    case fail:
      data_->p_end      = chrono::system_clock::now();
      data_->p_progress = {1, 1};
      break;
  }
  data_->p_state = in_state;
}

std::string_view process_message::trace_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->trace_;
}
std::string_view process_message::debug_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->debug_;
}
std::string_view process_message::info_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->info_;
}
std::string_view process_message::warn_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->warn_;
}
std::string_view process_message::err_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->err_;
}
std::string_view process_message::critical_log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->critical_;
}

rational_int process_message::get_progress() const {
  //  std::lock_guard _lock{_mutex};
  return data_->p_progress;
}
const process_message::state& process_message::get_state() const {
  //  std::lock_guard _lock{_mutex};
  return data_->p_state;
}
chrono::sys_time_pos::duration process_message::get_time() const {
  //  std::lock_guard _lock{_mutex};

  switch (data_->p_state) {
    case state::wait:
      return chrono::sys_time_pos::duration{0};
    case state::run:
      return chrono::system_clock::now() - data_->p_time;
    case state::fail:
    case state::success:
      return data_->p_end - data_->p_time;
  }
  return {};
}
const std::string& process_message::message_back() const { return data_->p_str_end; }

const std::string& process_message::get_name_id() const { return data_->p_name_id; }
void process_message::progress_clear() {
  std::lock_guard const _lock{data_->_mutex};
  data_->p_progress = 0;
}

}  // namespace doodle
