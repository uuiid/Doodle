#pragma once
#include <doodle_core/exception/exception.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace doodle {

// namespace details {
class DOODLE_CORE_EXPORT thread_pool : public details::no_copy {
 public:
  explicit thread_pool(size_t);
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type>;
  ~thread_pool();

  // need to keep track of threads so we can join them
  boost::asio::thread_pool pool_;
};
inline thread_pool::thread_pool(size_t threads)
    : pool_(threads) {
}
template <class F, class... Args>
[[nodiscard]] auto thread_pool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;
  return boost::asio::post(pool_,
                           std::packaged_task<return_type()>{
                               std::bind(std::forward<F>(f),
                                         std::forward<Args>(args)...)});
}
inline thread_pool::~thread_pool() {
  pool_.stop();
}
//}  // namespace details

// using ThreadPool = details::ThreadPool;
}  // namespace doodle
