#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>

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

class DOODLELIB_API ThreadPool : public details::no_copy {
 public:
  explicit ThreadPool(size_t);
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type>;
  ~ThreadPool();

 private:
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers;
  // the task queue
  std::queue<std::function<void()> > tasks;

  // synchronization
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false) {
  for (size_t i = 0; i < threads; ++i)
    workers.emplace_back(
        [this] {
          for (;;) {
            std::function<void()> task;

            {
              std::unique_lock<std::mutex> lock(this->queue_mutex);
              this->condition.wait(lock,
                                   [this] { return this->stop || !this->tasks.empty(); });
              if (this->stop && this->tasks.empty())
                return;
              task = std::move(this->tasks.front());
              this->tasks.pop();
            }

            task();
          }
        });
}

// add new work item to the pool
template <class F, class... Args>
[[nodiscard]] auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;

  auto task = new_object<std::packaged_task<return_type()> >(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // don't allow enqueueing after stopping the pool
    if (stop)
      throw DoodleError("enqueue on stopped ThreadPool");

    tasks.emplace([task]() { (*task)(); });
  }
  condition.notify_one();
  return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for (std::thread& worker : workers)
    worker.join();
}
namespace details {
class DOODLELIB_API ThreadPool : public details::no_copy {
 public:
  explicit ThreadPool(size_t);
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type>;
  ~ThreadPool();

 private:
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers;
  std::atomic_bool stop;
  boost::asio::io_context io_context;
  boost::asio::any_io_executor io_work;
};
inline ThreadPool::ThreadPool(size_t threads)
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
[[nodiscard]] auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;

  auto task = new_object<std::packaged_task<return_type()> >(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  std::future<return_type> res = task->get_future();
  boost::asio::post(io_context, [task]() { (*task)(); });
  return res;
}
inline ThreadPool::~ThreadPool() {
  io_work = decltype(io_work){};
  io_context.stop();
  for (auto& worker : workers)
    worker.join();
}
}  // namespace details
}  // namespace doodle
