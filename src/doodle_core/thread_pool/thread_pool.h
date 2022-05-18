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

  boost::asio::io_context& get_io_context() {
    return io_context;
  };

 private:
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers;
  std::atomic_bool stop;
  boost::asio::io_context io_context;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> io_work;
};
inline thread_pool::thread_pool(size_t threads)
    : stop(false),
      io_context(),
      io_work(boost::asio::make_work_guard(io_context)) {
  for (size_t i = 0; i < threads; ++i)
    workers.emplace_back(
        [this]() {
          this->io_context.run();
        });
}
template <class F, class... Args>
[[nodiscard]] auto thread_pool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;
  return boost::asio::post(io_context,
                           std::packaged_task<return_type()>{
                               std::bind(std::forward<F>(f),
                                         std::forward<Args>(args)...)});
}
inline thread_pool::~thread_pool() {
  io_work.reset();
  io_context.stop();
  for (auto& worker : workers)
    worker.join();
}
//}  // namespace details

// using ThreadPool = details::ThreadPool;
}  // namespace doodle
