#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>

#include <boost/signals2.hpp>

namespace doodle {
/**
 * @brief 长时间任务时， 使用这个类进行通知；
 *
 */
class DOODLELIB_API long_term : public details::no_copy {
  /**
   * @brief 其他线程运行结果（void 是闭包的包装， 只是用来确定完成结果的）
   *
   */
  bool p_fulfil;
  std::string p_str;
  std::double_t p_progress;
  std::mutex _mutex;
  std::size_t p_num;
  // std::recursive_mutex _mutex;
  std::vector<long_term_ptr> p_child;
 public:
  long_term();
  virtual ~long_term();
  /**
   * @brief 将信号转发到传入的新的信号中去
   * @param in_forward 新的信号
   */
  void forward_sig(const long_term_ptr& in_forward);
  void forward_sig(const std::vector<long_term_ptr>& in_forward);

  std::double_t step(std::double_t in_);
  /**
   * @brief 这个是步进信号
   * @param std::double_t 每次步进的大小
   *
   */
  boost::signals2::signal<void(std::double_t)> sig_progress;
  /**
   * @brief 结果信号
   *
   */
  boost::signals2::signal<void(const std::string& message)> sig_message_result;
  /**
   * @brief 完成信号, 完成信号要在结果信息之后发出
   *
   */
  boost::signals2::signal<void()> sig_finished;

  [[nodiscard]] bool fulfil() const;
  [[nodiscard]] std::string message_result() const;

  std::vector<std::shared_future<void>> p_list;
};

}  // namespace doodle
