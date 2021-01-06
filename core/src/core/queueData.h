#pragma once
#include <core_global.h>
#include <future>

#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S
class CORE_API queueData {
  using futureB = std::future<bool>;

 public:
  queueData(futureB&& f);

  // boost::signals2::signal<void(int)> progress;

  // 提交信息
  bool submit();

  //添加信息
  void appendInfo(const std::string& str) noexcept;
  const std::string& Info() const noexcept;

  const int& progress() const noexcept;

  // 获得异步结果
  const futureB& future() const noexcept;

  // 设置名称(标题)
  void setName(const std::string& name) noexcept;
  const std::string& Name() const noexcept;

 private:
  std::future<bool> p_fu;
  std::string p_name;
  std::string p_string;
  int p_progress;
};

DOODLE_NAMESPACE_E