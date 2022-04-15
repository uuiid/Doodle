#pragma once
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
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
class DOODLELIB_API thread_pool : public details::no_copy {
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
  boost::asio::any_io_executor io_work;
};
inline thread_pool::thread_pool(size_t threads)
    : stop(false),
      io_context(),
      io_work(
          boost::asio::require(
              io_context.get_executor(),
              boost::asio::execution::outstanding_work.tracked)) {
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

  auto task         = std::make_shared<std::packaged_task<return_type()> >(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  std::future<return_type> res = task->get_future();
  boost::asio::post(io_context, [task]() { (*task)(); });
  return res;
}
inline thread_pool::~thread_pool() {
  io_work = decltype(io_work){};
  io_context.stop();
  for (auto& worker : workers)
    worker.join();
}
//}  // namespace details

// using ThreadPool = details::ThreadPool;
}  // namespace doodle
