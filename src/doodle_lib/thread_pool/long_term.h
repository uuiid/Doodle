#pragma once

#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/rational.hpp>
#include <boost/signals2.hpp>
#include <optional>
namespace doodle {

using rational_int = boost::rational<std::size_t>;
/**
 * @brief 长时间任务时， 使用这个类进行通知；
 *
 */
class DOODLELIB_API long_term
    : public details::no_copy,
      public std::enable_shared_from_this<long_term> {
 public:
  enum state {
    none_   = 0,
    success = 1,
    fail    = 2,
    wait    = 3,
    run     = 4
  };
  enum level {
    info    = 0,
    warning = 1,
  };

 private:
  /**
   * @brief 其他线程运行结果（void 是闭包的包装， 只是用来确定完成结果的）
   *
   */
  bool p_fulfil;
  std::string p_id;
  std::string p_name;
  chrono::sys_time_pos p_time;
  std::optional<chrono::sys_time_pos> p_end;
  state p_state;

  std::deque<std::string> p_str;
  std::deque<std::string> p_log;

  rational_int p_progress;
  std::mutex _mutex;
  // std::recursive_mutex _mutex;
  std::vector<long_term_ptr> p_child;

 public:
  long_term();
  virtual ~long_term();

  void post_constructor();

  std::string& get_name();
  void set_name(const std::string& in_string);

  std::string& get_id();

  rational_int step(rational_int in_);
  /**
   * @brief 这个是步进信号
   * @param std::double_t 每次步进的大小
   *
   */
  boost::signals2::signal<void(rational_int)> sig_progress;
  /**
   * @brief 结果信号
   *
   */
  boost::signals2::signal<void(const std::string& message, level)> sig_message_result;

  /**
   * @brief 完成信号, 完成信号要在结果信息之后发出
   *
   */
  boost::signals2::signal<void()> sig_finished;

  void start();
  void set_state(long_term::state in_state);

  [[nodiscard]] bool fulfil() const;
  [[nodiscard]] std::string message_result() const;
  [[nodiscard]] const std::deque<std::string>& message() const;
  [[nodiscard]] const std::deque<std::string>& log() const;

  [[nodiscard]] rational_int get_progress() const;
  [[nodiscard]] std::double_t get_progress_int() const;

  [[nodiscard]] std::string_view get_state_str() const;
  [[nodiscard]] std::string get_time_str() const;
  std::vector<std::shared_future<void>> p_list;
};

class DOODLELIB_API process_message {
 public:
  enum level {
    info    = 0,
    warning = 1,
  };
  enum state {
    success = 1,
    fail    = 2,
    wait    = 3,
    run     = 4
  };

 private:
  chrono::sys_time_pos p_time;
  std::optional<chrono::sys_time_pos> p_end;
  std::string p_err;
  std::string p_log;
  std::string p_str_end;
  std::string p_name;
  std::string p_name_id;
  state p_state;
  rational_int p_progress;
  std::mutex _mutex;

 public:
  process_message();

  process_message(process_message&&) noexcept;
  process_message& operator=(process_message&&) noexcept;
  process_message(const process_message&) noexcept;
  process_message& operator=(const process_message&) noexcept;

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_name_id() const;
  void set_name(const std::string& in_string);

  void progress_step(const rational_int& in_rational_int);
  void message(const string& in_string, const level& in_level_enum);
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
};

}  // namespace doodle
