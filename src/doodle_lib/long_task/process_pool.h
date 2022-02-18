//
// Created by TD on 2021/12/28.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
template <class Delta>
class bounded_pool {
  struct process_handler {
    using instance_type  = std::unique_ptr<void, void (*)(void *)>;
    using update_fn_type = bool(process_handler &, Delta, void *);
    using abort_fn_type  = void(process_handler &, bool);
    using next_type      = std::unique_ptr<process_handler>;

    instance_type instance;
    update_fn_type *update;
    abort_fn_type *abort;
    next_type next;
  };

  struct continuation {
    continuation(process_handler *ref)
        : handler{ref} {}

    template <typename Proc, typename... Args>
    continuation then(Args &&...args) {
      static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
      auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &bounded_pool::deleter<Proc>};
      handler->next.reset(new process_handler{std::move(proc), &bounded_pool::update<Proc>, &bounded_pool::abort<Proc>, nullptr});
      handler = handler->next.get();
      return *this;
    }

    template <typename Func>
    continuation then(Func &&func) {
      return then<entt::process_adaptor<std::decay_t<Func>, Delta>>(std::forward<Func>(func));
    }

   private:
    process_handler *handler;
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
  using size_type               = std::size_t;

  /*! @brief Default constructor. */
  bounded_pool()                = default;

  /*! @brief Default move constructor. */
  bounded_pool(bounded_pool &&) = default;

  /*! @brief Default move assignment operator. @return This bounded_pool. */
  bounded_pool &operator=(bounded_pool &&) = default;

  /**
   * @brief Number of processes currently scheduled.
   * @return Number of processes currently scheduled.
   */
  [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
    return handlers.size();
  }

  /**
   * @brief Returns true if at least a process is currently scheduled.
   * @return True if there are scheduled processes, false otherwise.
   */
  [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
    return handlers.empty();
  }

  /**
   * @brief Discards all scheduled processes.
   *
   * Processes aren't aborted. They are discarded along with their children
   * and never executed again.
   */
  void clear() {
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
    static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
    auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &bounded_pool::deleter<Proc>};
    process_handler handler{std::move(proc), &bounded_pool::update<Proc>, &bounded_pool::abort<Proc>, nullptr};
    // forces the process to exit the uninitialized state
    //    handler.update(handler, {}, nullptr);
    return continuation{&handlers.emplace_back(std::move(handler))};
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
    auto sz = handlers.size();

    for (std::size_t pos = (((std::int64_t)max_process > sz) ? sz : (std::int64_t)max_process); pos; --pos) {
      auto &handler = handlers[pos - 1];

      if (const auto dead = handler.update(handler, delta, data); dead) {
        std::swap(handler, handlers[--sz]);
      }
    }

    handlers.erase(handlers.begin() + sz, handlers.end());
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
    decltype(handlers) exec;
    exec.swap(handlers);

    for (auto &&handler : exec) {
      handler.abort(handler, immediately);
    }

    std::move(handlers.begin(), handlers.end(), std::back_inserter(exec));
    handlers.swap(exec);
  }

  void set_bounded(const std::int16_t in_int_16) {
    max_process = in_int_16;
  }

  template <typename Proc, typename... Args>
  static void wait(Args &&...args) {
    bounded_pool k_b{};
    k_b.set_bounded(std::thread::hardware_concurrency());
    k_b.template attach<Proc>(std::forward<Args>(args)...);
    while (!k_b.empty())
      k_b.update({}, nullptr);
  };
  //  template <typename Proc, typename... Args>
  //  static void wait(Args &&...args) {
  //    wait<Proc>(nullptr, std::forward<Args>(args)...);
  //  };

 private:
  std::vector<process_handler> handlers{};
  std::atomic_int16_t max_process{};
};

/**
 * @brief Cooperative scheduler for processes.
 *
 * A cooperative scheduler runs processes and helps managing their life cycles.
 *
 * Each process is invoked once per tick. If a process terminates, it's
 * removed automatically from the scheduler and it's never invoked again.<br/>
 * A process can also have a child. In this case, the process is replaced with
 * its child when it terminates if it returns with success. In case of errors,
 * both the process and its child are discarded.
 *
 * Example of use (pseudocode):
 *
 * @code{.cpp}
 * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
 *     // code
 * }).then<my_process>(arguments...);
 * @endcode
 *
 * In order to invoke all scheduled processes, call the `update` member function
 * passing it the elapsed time to forward to the tasks.
 *
 * @sa process
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template <typename Delta>
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
  };

  struct continuation {
    continuation(process_handler *ref)
        : handler{ref} {}

    template <typename Proc, typename... Args>
    continuation then(Args &&...args) {
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

   private:
    process_handler *handler;
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
  using size_type         = std::size_t;

  /*! @brief Default constructor. */
  scheduler()             = default;

  /*! @brief Default move constructor. */
  scheduler(scheduler &&) = default;

  /*! @brief Default move assignment operator. @return This scheduler. */
  scheduler &operator=(scheduler &&) = default;

  /**
   * @brief Number of processes currently scheduled.
   * @return Number of processes currently scheduled.
   */
  [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
    return handlers.size();
  }

  /**
   * @brief Returns true if at least a process is currently scheduled.
   * @return True if there are scheduled processes, false otherwise.
   */
  [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
    return handlers.empty() && handlers_next.empty();
  }

  /**
   * @brief Discards all scheduled processes.
   *
   * Processes aren't aborted. They are discarded along with their children
   * and never executed again.
   */
  void clear() {
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
    //    static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
    auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &scheduler::deleter<Proc>};
    process_handler handler{std::move(proc), &scheduler::update<Proc>, &scheduler::abort<Proc>, nullptr};
    // forces the process to exit the uninitialized state
    handler.update(handler, {}, nullptr);
    return continuation{&handlers_next.emplace_back(std::move(handler))};
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
    auto sz = handlers.size();

    for (auto pos = handlers.size(); pos; --pos) {
      auto &handler = handlers[pos - 1];

      if (const auto dead = handler.update(handler, delta, data); dead) {
        std::swap(handler, handlers[--sz]);
      }
    }

    handlers.erase(handlers.begin() + sz, handlers.end());

    std::move(handlers_next.begin(), handlers_next.end(), std::back_inserter(handlers));
    handlers_next.clear();
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
    decltype(handlers) exec;
    exec.swap(handlers);

    for (auto &&handler : exec) {
      handler.abort(handler, immediately);
    }

    std::move(handlers.begin(), handlers.end(), std::back_inserter(exec));
    handlers.swap(exec);
  }

 private:
  std::vector<process_handler> handlers{};
  decltype(handlers) handlers_next{};
};

class DOODLELIB_API null_process_t : public process_t<null_process_t> {
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
class DOODLELIB_API one_process_t : public process_t<one_process_t> {
 public:
  std::function<void()> one_loop;
  explicit one_process_t(const std::function<void()> &in_function)
      : one_loop(in_function){

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
