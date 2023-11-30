#include "process_message.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/logger/logger.h>

#include <boost/locale.hpp>
#include <boost/numeric/conversion/cast.hpp>

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

    switch (msg.level) {
      case spdlog::level::level_enum::trace:
      case spdlog::level::level_enum::debug:
      case spdlog::level::level_enum::info:
        data_->p_log += fmt::to_string(formatted);
        break;
      case spdlog::level::level_enum::warn:
      case spdlog::level::level_enum::err:
      case spdlog::level::level_enum::critical:
      case spdlog::level::level_enum::off:
        data_->p_err += fmt::to_string(formatted);
        break;
      case spdlog::level::level_enum::n_levels:
        break;
    }
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

  data_->p_logger  = g_logger_ctrl().make_log(in_name);
  data_->p_logger->sinks().emplace_back(std::make_shared<process_message_sink_mt>(this->data_));
}

const std::string& process_message::get_name() const { return data_->p_name; }
void process_message::set_name(const std::string& in_string) {
  std::lock_guard _lock{data_->_mutex};
  data_->p_name    = in_string;
  data_->p_name_id = fmt::format("{}##{}", get_name(), fmt::ptr(this));
}
void process_message::progress_step(const rational_int& in_rational_int) {
  std::lock_guard _lock{data_->_mutex};
  data_->p_progress += in_rational_int;
  if (data_->p_progress > 1) --data_->p_progress;
}
void process_message::message(const std::string& in_string, const level& in_level_enum) {
  auto l_msg{in_string};

  static boost::locale::generator k_gen{};
  //  k_gen.categories(boost::locale::all_categories ^ boost::locale::formatting_facet ^ boost::locale::parsing_facet);

  static auto l_local{k_gen("zh_CN.UTF-8")};
  if (ranges::all_of(l_msg, [&](const std::string::value_type& in_type) -> bool {
        return std::isspace(in_type, l_local);
      })) {
    return;
  }
  //  l_msg |= ranges::actions::remove_if([](const std::string::value_type& in_type) -> bool {
  //    return std::isspace(in_type, l_local);
  //  });
  spdlog::info(l_msg);
  if (l_msg.back() != '\n') {
    l_msg += '\n';
  }

  std::lock_guard l_lock{data_->_mutex};
  switch (in_level_enum) {
    case level::warning:
      data_->p_err += l_msg;
      data_->p_str_end = l_msg;
      break;
    default:
      data_->p_log += l_msg;
      break;
  }
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
std::string_view process_message::err() const {
  //  std::lock_guard _lock{_mutex};
  return data_->p_err;
}

std::string_view process_message::log() const {
  //  std::lock_guard _lock{_mutex};
  return data_->p_log;
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
  std::lock_guard _lock{data_->_mutex};
  data_->p_progress = 0;
}

}  // namespace doodle
