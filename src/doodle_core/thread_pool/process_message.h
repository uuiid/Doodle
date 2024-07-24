#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/rational.hpp>
#include <boost/signals2.hpp>

#include <magic_enum.hpp>

namespace doodle {
using rational_int = boost::rational<std::size_t>;

namespace details {
struct logger_Buffer {
  enum state { success = 1, fail = 2, wait = 3, run = 4, pause = 5 };

  std::array<std::string, 6> loggers_;
  rational_int progress_;
  state state_{logger_Buffer::wait};
  std::string end_str_;
  chrono::sys_time_pos::duration extra_time_{0};

  chrono::sys_time_pos start_time_;
  chrono::sys_time_pos end_time_;
};

template <class Mutex>
class process_message_sink;
using process_message_sink_mt = details::process_message_sink<std::mutex>;
}

class DOODLE_CORE_API process_message {
public:
  enum level {
    info = 0,
    warning = 1,
  };

  using state = details::logger_Buffer::state;

private:
  struct data_t {
    std::string p_name;
    std::string p_name_id;
    boost::asio::cancellation_signal cancel_sig;
    logger_ptr p_logger;
    std::shared_ptr<details::process_message_sink_mt> sink_;
  };

  std::shared_ptr<data_t> data_;

  template <class Mutex>
  friend class details::process_message_sink;

public:
  explicit process_message(std::string in_name);

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_name_id() const;

  void progress_clear();
  [[nodiscard]] std::string message_back() const;

  [[nodiscard]] std::string level_log(const level in_level) const;

  [[nodiscard]] rational_int get_progress() const;

  [[nodiscard]] inline std::double_t get_progress_f() const {
    return boost::rational_cast<std::double_t>(get_progress() * rational_int{100});
  };
  [[nodiscard]] const state& get_state() const;
  [[nodiscard]] chrono::sys_time_pos::duration get_time() const;

  [[nodiscard]] inline bool is_run() const { return get_state() == state::run; }
  [[nodiscard]] inline bool is_wait() const { return get_state() == state::wait; }
  [[nodiscard]] inline bool is_success() const { return get_state() == state::success; }
  [[nodiscard]] inline bool is_fail() const { return get_state() == state::fail; }
  [[nodiscard]] logger_ptr logger() const;

  boost::signals2::signal<void()> aborted_sig;
  inline auto get_cancel_slot() { return data_->cancel_sig.slot(); }

  inline void aborted() {
    aborted_sig();

    data_->cancel_sig.emit(boost::asio::cancellation_type::all);
  }
};

inline auto format_as(process_message::state f) { return magic_enum::enum_name(f); }
} // namespace doodle