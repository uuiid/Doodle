//
// Created by TD on 2021/12/28.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <utility>
namespace doodle {

namespace pool_n {
class bounded_limiter {
 private:
  std::atomic_int16_t max_process{};

 public:
  template <typename List_T>
  std::size_t operator()(const List_T &in_list) const {
    auto sz           = in_list.size();
    std::size_t l_max = max_process;
    return (l_max > sz) ? sz : l_max;
  }

  template <typename Num_T, std::enable_if_t<std::is_arithmetic_v<Num_T>, bool> = true>
  bounded_limiter &operator=(Num_T in) {
    max_process = boost::numeric_cast<std::int16_t>(in);
    return *this;
  };
};

class null_limiter {
 private:
 public:
  template <typename List_T>
  std::size_t operator()(const List_T &in_list) const {
    return in_list.size();
  }
};
}  // namespace pool_n

/**
 * @brief Cooperative scheduler for processes.
 * 协作调度器运行进程并帮助管理它们的生命周期。
 * 每个进程更新异常。如果一个进程终止，它将被自动从调度程序中删除，并且不再调用它
 *
 * 一个过程也可以有一个孩子。在这种情况下，该过程被替换为
 *
 * 如果成功返回，则终止。如果出现错误， 进程及其子进程都将被丢弃。
 * Example of use (pseudocode):
 *
 * @code{.cpp}
 * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
 *     // code
 * }).then<my_process>(arguments...);
 * @endcode
 *
 * 为了调用所有计划的进程，调用'update'成员函数，将转发到任务所用的时间传递给它。
 *
 * @tparam 时间类
 * @tparam Timiter 限制器类
 */
template <typename Delta, typename Timiter>
class scheduler {
  struct process_handler {
    using instance_type  = std::unique_ptr<void, void (*)(void *)>;
    using update_fn_type = bool(process_handler &, Delta, void *);
    using abort_fn_type  = void(process_handler &, bool);
    using next_type      = std::unique_ptr<process_handler>;

    instance_type instance;
    update_fn_type *update;
    abort_fn_type *abort;
    next_type next;
    bool operator==(const process_handler &in_rhs) const {
      return std::tie(instance, update, abort, next) == std::tie(in_rhs.instance, in_rhs.update, in_rhs.abort, in_rhs.next);
    }
    bool operator!=(const process_handler &in_rhs) const {
      return !(in_rhs == *this);
    }
  };

  /**
   * @brief 匿名连接对象
   *
   */
  struct continuation {
    continuation(scheduler *in_self, process_handler *ref)
        : handler{ref},
          self_{in_self} {}

    template <typename Proc, typename... Args>
    continuation then(Args &&...args) {
      std::lock_guard l_g{self_->mutex_};
      //      static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
      auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &scheduler::deleter<Proc>};
      handler->next.reset(new process_handler{std::move(proc), &scheduler::update<Proc>, &scheduler::abort<Proc>, nullptr});
      handler = handler->next.get();
      return *this;
    }

    template <typename Func>
    continuation then(Func &&func) {
      return then<entt::process_adaptor<std::decay_t<Func>, Delta>>(std::forward<Func>(func));
    }
    /**
     * @brief 初始化时的过程句柄
     */
    template <typename Proc>
    Proc *get() {
      return static_cast<Proc *>(handler->instance.get());
    }
    process_handler *handler;

   private:
    scheduler *self_;
  };

  template <typename Proc>
  [[nodiscard]] static bool update(process_handler &handler, const Delta delta, void *data) {
    auto *process = static_cast<Proc *>(handler.instance.get());
    process->tick(delta, data);

    if (process->rejected()) {
      return true;
    } else if (process->finished()) {
      if (handler.next) {
        handler = std::move(*handler.next);
        // forces the process to exit the uninitialized state
        return handler.update(handler, {}, nullptr);
      }

      return true;
    }

    return false;
  }

  template <typename Proc>
  static void abort(process_handler &handler, const bool immediately) {
    static_cast<Proc *>(handler.instance.get())->abort(immediately);
  }

  template <typename Proc>
  static void deleter(void *proc) {
    delete static_cast<Proc *>(proc);
  }

 public:
  /*! @brief Unsigned integer type. */
  using size_type                             = std::size_t;

  /*! @brief Default constructor. */
  scheduler()                                 = default;

  /*! @brief Default move constructor. */
  scheduler(scheduler &&) noexcept            = default;

  /*! @brief Default move assignment operator. @return This scheduler. */
  scheduler &operator=(scheduler &&) noexcept = default;

  /**
   * @brief Number of processes currently scheduled.
   * @return Number of processes currently scheduled.
   */
  [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
    //    std::lock_guard l_g{mutex_};
    return handlers.size();
  }

  /**
   * @brief 测试是否为空, 会测试所有的列表
   * @return True if there are scheduled processes, false otherwise.
   */
  [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
    //    std::lock_guard l_g{mutex_};
    return handlers.empty() && handlers_next.empty();
  }

  /**
   * @brief Discards all scheduled processes.
   *
   * Processes aren't aborted. They are discarded along with their children
   * and never executed again.
   */
  void clear() {
    std::lock_guard l_g{mutex_};
    handlers.clear();
  }

  /**
   * @brief Schedules a process for the next tick.
   *
   * Returned value is an opaque object that can be used to attach a child to
   * the given process. The child is automatically scheduled when the process
   * terminates and only if the process returns with success.
   *
   * Example of use (pseudocode):
   *
   * @code{.cpp}
   * // schedules a task in the form of a process class
   * scheduler.attach<my_process>(arguments...)
   * // appends a child in the form of a lambda function
   * .then([](auto delta, void *, auto succeed, auto fail) {
   *     // code
   * })
   * // appends a child in the form of another process class
   * .then<my_other_process>();
   * @endcode
   *
   * @tparam Proc Type of process to schedule.
   * @tparam Args Types of arguments to use to initialize the process.
   * @param args Parameters to use to initialize the process.
   * @return An opaque object to use to concatenate processes.
   */
  template <typename Proc, typename... Args>
  auto attach(Args &&...args) {
    std::lock_guard l_g{mutex_};
    //    static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
    auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &scheduler::deleter<Proc>};
    process_handler handler{std::move(proc), &scheduler::update<Proc>, &scheduler::abort<Proc>, nullptr};
    // forces the process to exit the uninitialized state
    // handler.update(handler, {}, nullptr);
    return continuation{this, &handlers_next.emplace_back(std::move(handler))};
  }

  /**
   * @brief Schedules a process for the next tick.
   *
   * A process can be either a lambda or a functor. The scheduler wraps both
   * of them in a process adaptor internally.<br/>
   * The signature of the function call operator should be equivalent to the
   * following:
   *
   * @code{.cpp}
   * void(Delta delta, void *data, auto succeed, auto fail);
   * @endcode
   *
   * Where:
   *
   * * `delta` is the elapsed time.
   * * `data` is an opaque pointer to user data if any, `nullptr` otherwise.
   * * `succeed` is a function to call when a process terminates with success.
   * * `fail` is a function to call when a process terminates with errors.
   *
   * The signature of the function call operator of both `succeed` and `fail`
   * is equivalent to the following:
   *
   * @code{.cpp}
   * void();
   * @endcode
   *
   * Returned value is an opaque object that can be used to attach a child to
   * the given process. The child is automatically scheduled when the process
   * terminates and only if the process returns with success.
   *
   * Example of use (pseudocode):
   *
   * @code{.cpp}
   * // schedules a task in the form of a lambda function
   * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
   *     // code
   * })
   * // appends a child in the form of another lambda function
   * .then([](auto delta, void *, auto succeed, auto fail) {
   *     // code
   * })
   * // appends a child in the form of a process class
   * .then<my_process>(arguments...);
   * @endcode
   *
   * @sa process_adaptor
   *
   * @tparam Func Type of process to schedule.
   * @param func Either a lambda or a functor to use as a process.
   * @return An opaque object to use to concatenate processes.
   */
  template <typename Func>
  auto attach(Func &&func) {
    using Proc = entt::process_adaptor<std::decay_t<Func>, Delta>;
    return attach<Proc>(std::forward<Func>(func));
  }

  /**
   * @brief Updates all scheduled processes.
   *
   * All scheduled processes are executed in no specific order.<br/>
   * If a process terminates with success, it's replaced with its child, if
   * any. Otherwise, if a process terminates with an error, it's removed along
   * with its child.
   *
   * @param delta Elapsed time.
   * @param data Optional data.
   */
  void update(const Delta delta, void *data = nullptr) {
    std::lock_guard l_g{mutex_};
    std::move(handlers_next.begin(), handlers_next.end(), std::back_inserter(handlers));
    handlers_next.clear();
    if (handlers.empty())
      return;
    auto l_end         = handlers.begin() + timiter_(handlers);
    auto l_erase_benin = std::remove_if(handlers.begin(), l_end, [&](typename decltype(this->handlers)::value_type &handler) {
      return handler.update(handler, delta, data);
    });
    if (l_erase_benin != handlers.end() &&
        l_erase_benin != l_end)
      handlers.erase(l_erase_benin, l_end);

    //    std::move(handlers_next.begin(), handlers_next.end(), std::back_inserter(handlers));
    //    handlers_next.clear();
  }

  /**
   * @brief Aborts all scheduled processes.
   *
   * Unless an immediate operation is requested, the abort is scheduled for
   * the next tick. Processes won't be executed anymore in any case.<br/>
   * Once a process is fully aborted and thus finished, it's discarded along
   * with its child, if any.
   *
   * @param immediately Requests an immediate operation.
   */
  void abort(const bool immediately = false) {
    std::lock_guard l_g{mutex_};
    for (auto &&handler : handlers) {
      handler.abort(handler, immediately);
    }
    for (auto &&handler : handlers_next) {
      handler.abort(handler, immediately);
    }
  }

  Timiter timiter_;

  template <typename Proc, typename... Args>
  static void wait(Args &&...args) {
    scheduler k_b{};
    k_b.timiter_ = std::thread::hardware_concurrency();
    k_b.template attach<Proc>(std::forward<Args>(args)...);
    while (!k_b.empty())
      k_b.update({}, nullptr);
  };

 private:
  std::vector<process_handler> handlers{};
  decltype(handlers) handlers_next{};
  std::recursive_mutex mutex_;
};

class DOODLE_CORE_EXPORT null_process_t : public process_t<null_process_t> {
 public:
  null_process_t() = default;
  using base_type  = process_t<null_process_t>;
  [[maybe_unused]] inline void init(){};
  [[maybe_unused]] inline void succeeded(){};
  [[maybe_unused]] inline void failed(){};
  [[maybe_unused]] inline void aborted(){};
  [[maybe_unused]] inline void update(base_type::delta_type, void *data) {
    this->succeed();
  };
};
class DOODLE_CORE_EXPORT one_process_t : public process_t<one_process_t> {
 public:
  std::function<void()> one_loop;
  explicit one_process_t(std::function<void()> in_function)
      : one_loop(std::move(in_function)){

        };
  using base_type = process_t<one_process_t>;
  [[maybe_unused]] inline void init(){};
  [[maybe_unused]] inline void succeeded(){};
  [[maybe_unused]] inline void failed(){};
  [[maybe_unused]] inline void aborted(){};
  [[maybe_unused]] inline void update(base_type::delta_type, void *data) {
    one_loop();
    this->succeed();
  };
};
}  // namespace doodle
