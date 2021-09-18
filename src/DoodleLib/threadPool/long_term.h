#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>

#include <boost/rational.hpp>
#include <boost/signals2.hpp>
#include <optional>
namespace doodle {

using rational_int = boost::rational<std::size_t>;
/**
 * @brief 长时间任务时， 使用这个类进行通知；
 *
 */
class DOODLELIB_API long_term : public details::no_copy {
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

  static long_term_ptr make_this_shared();

  std::string& get_name();
  void set_name(const std::string& in_string);

  std::string& get_id();
  /**
   * @brief 将信号转发到传入的新的信号中去
   * @param in_forward 新的信号
   */
  void forward_sig(const long_term_ptr& in_forward);
  void forward_sig(const std::vector<long_term_ptr>& in_forward);

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
  [[nodiscard]] const std::deque<std::string> message() const;
  [[nodiscard]] const std::deque<std::string> log() const;

  [[nodiscard]] rational_int get_progress() const;
  [[nodiscard]] std::double_t get_progress_int() const;

  [[nodiscard]] std::string_view get_state_str() const;
  [[nodiscard]] std::string get_time_str() const;
  std::vector<std::shared_future<void>> p_list;
};

}  // namespace doodle
