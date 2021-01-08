#pragma once
#include <core_global.h>
#include <future>

#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S
class CORE_API queueData : public std::enable_shared_from_this<queueData> {
  using futureB = std::future<bool>;

 public:
  //在每次创建出来时都会自动提交到队列管理器中
  explicit queueData(futureB& f);

  boost::signals2::signal<void()> ProgressChanged;
  boost::signals2::signal<void(std::string)> InfoChanged;
  boost::signals2::signal<void()> finishEd;
  //添加信息
  void appendInfo(const std::string& str) noexcept;
  const std::string& Info() const noexcept;

  const int& Progress() const noexcept;
  void setProgress(const int value) noexcept;

  // 获得异步结果
  const futureB& Future() const noexcept;

  // 设置名称(标题)
  void setName(const std::string& name) noexcept;
  const std::string& Name() const noexcept;

  // 提交信息
  void submit();

 private:
  std::future<bool> p_fu;
  std::string p_name;
  std::string p_string;
  int p_progress;
};
DOODLE_NAMESPACE_E