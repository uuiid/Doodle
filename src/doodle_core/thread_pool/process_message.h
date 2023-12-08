#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/rational.hpp>
#include <boost/signals2.hpp>

#include <magic_enum.hpp>
namespace doodle {

using rational_int = boost::rational<std::size_t>;
namespace details {
template <class Mutex>
class process_message_sink;
}
class DOODLE_CORE_API process_message {
 public:
  enum level {
    info    = 0,
    warning = 1,
  };
  enum state { success = 1, fail = 2, wait = 3, run = 4 };

 private:
  struct data_t {
    chrono::sys_time_pos p_time;
    chrono::sys_time_pos p_end;
    std::string p_err;
    std::string p_log;
    std::string p_str_end;
    std::string p_name;
    std::string p_name_id;
    state p_state;
    rational_int p_progress;
    std::mutex _mutex;

    logger_ptr p_logger;
  };
  std::shared_ptr<data_t> data_;

  template <class Mutex>
  friend class details::process_message_sink;

 public:
  explicit process_message(std::string in_name);

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_name_id() const;

  void progress_step(const rational_int& in_rational_int);
  void progress_clear();
  [[nodiscard]] const std::string& message_back() const;

  void set_state(state in_state);
  [[nodiscard]] std::string_view err() const;
  [[nodiscard]] std::string_view log() const;

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

  boost::asio::cancellation_signal cancel_sig;
  boost::signals2::signal<void()> aborted_sig;

  inline void aborted() {
    aborted_sig();
    cancel_sig.emit(boost::asio::cancellation_type::all);
  }
};
inline auto format_as(process_message::state f) { return magic_enum::enum_name(f); }
}  // namespace doodle
