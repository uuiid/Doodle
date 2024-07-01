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

    std::string trace_;
    std::string debug_;
    std::string info_;
    std::string warn_;
    std::string err_;
    std::string critical_;

    std::string p_str_end;
    std::string p_name;
    std::string p_name_id;
    state p_state;
    rational_int p_progress;
    std::mutex _mutex;
    boost::asio::cancellation_signal cancel_sig;

    logger_ptr p_logger;

    // 额外的时间段
    chrono::sys_time_pos::duration p_extra_time{0};
  };
  std::shared_ptr<data_t> data_;

  template <class Mutex>
  friend class details::process_message_sink;

 public:
  class log_msg_guard {
    std::shared_ptr<std::lock_guard<std::mutex>> lock_;

   public:
    std::string_view msg_;
    log_msg_guard() = default;
    explicit log_msg_guard(std::shared_ptr<std::lock_guard<std::mutex>> in_lock, std::string_view in_msg)
        : lock_(std::move(in_lock)), msg_(in_msg) {}
  };
  explicit process_message(std::string in_name);

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_name_id() const;

  void progress_clear();
  [[nodiscard]] std::string message_back() const;

  void set_state(state in_state);
  [[nodiscard]] std::string trace_log() const;
  [[nodiscard]] std::string debug_log() const;
  [[nodiscard]] std::string info_log() const;
  [[nodiscard]] std::string warn_log() const;
  [[nodiscard]] std::string err_log() const;
  [[nodiscard]] std::string critical_log() const;

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
}  // namespace doodle
