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

  static constexpr std::size_t g_max_size = 1024 * 100;
  static constexpr std::size_t l_end_size{120};
  using log_str     = std::array<char, g_max_size>;
  using log_end_str = std::array<char, l_end_size>;
  std::array<log_str, 6> loggers_;
  log_end_str end_str_;
  std::atomic<rational_int> progress_;
  std::atomic<state> state_{logger_Buffer::wait};
  std::atomic<chrono::sys_time_pos::duration> extra_time_{};

  std::atomic<chrono::sys_time_pos> start_time_;
  std::atomic<chrono::sys_time_pos> end_time_;
};

template <class Mutex>
class process_message_sink;
using process_message_sink_mt = details::process_message_sink<std::mutex>;
}

class DOODLE_CORE_API process_message {
public:
  using state = details::logger_Buffer::state;

private:
  struct data_t {
    std::string p_name;
    std::string p_name_id;
    boost::asio::cancellation_signal cancel_sig;
    logger_ptr p_logger;
    boost::signals2::scoped_connection scoped_connection_;
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
  [[nodiscard]] const details::logger_Buffer::log_end_str& message_back() const;

  [[nodiscard]] const details::logger_Buffer::log_str& level_log(const level::level_enum in_level) const;

  [[nodiscard]] rational_int get_progress() const;

  [[nodiscard]] inline std::double_t get_progress_f() const {
    return boost::rational_cast<std::double_t>(get_progress() * rational_int{100});
  };
  [[nodiscard]] state get_state() const;
  [[nodiscard]] chrono::sys_time_pos::duration get_time() const;

  [[nodiscard]] inline bool is_run() const { return get_state() == state::run; }
  [[nodiscard]] inline bool is_wait() const { return get_state() == state::wait; }
  [[nodiscard]] inline bool is_success() const { return get_state() == state::success; }
  [[nodiscard]] inline bool is_fail() const { return get_state() == state::fail; }
  [[nodiscard]] logger_ptr logger() const;

  inline auto get_cancel_slot() { return data_->cancel_sig.slot(); }

  bool is_connected() const { return data_->cancel_sig.slot().has_handler(); }

  inline void aborted() {
    data_->cancel_sig.emit(boost::asio::cancellation_type::all);
  }
};

inline auto format_as(process_message::state f) { return magic_enum::enum_name(f); }
} // namespace doodle