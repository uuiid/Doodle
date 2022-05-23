//
// Created by TD on 2022/5/23.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <utility>

#include <boost/asio/strand.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>

//#include <boost/intrusive/list.hpp>
//#include <boost/intrusive/list_hook.hpp>

namespace doodle {
template <typename Delta>
class DOODLE_CORE_EXPORT asio_pool {
 private:
  //  using list_base_hook = boost::intrusive::list_base_hook<
  //      boost::intrusive::link_mode<
  //          boost::intrusive::auto_unlink>>;

  struct process_handler {
    using instance_type  = std::unique_ptr<void, void (*)(void *)>;
    using update_fn_type = bool(process_handler &, Delta, void *);
    using abort_fn_type  = void(process_handler &, bool);
    using next_type      = std::unique_ptr<process_handler>;

    explicit process_handler(
        instance_type &&in_instance,
        update_fn_type *in_update,
        abort_fn_type *in_abort,
        next_type in_next)
        : instance(std::move(in_instance)),
          update(in_update),
          abort(in_abort),
          next(in_next) {}

    process_handler(process_handler &&) noexcept            = default;
    process_handler &operator=(process_handler &&) noexcept = default;
    ~process_handler()                                      = default;
    /**
     * @brief 删除复制函数
     */
    process_handler(process_handler &) noexcept             = delete;
    /**
     * @brief 删除复制函数
     */
    process_handler &operator=(process_handler &) noexcept  = delete;
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

  //  using handle_list = boost::intrusive::list<process_handler,
  //                                             boost::intrusive::constant_time_size<false>>;
  using handle_list = std::set<process_handler>;

  template <typename Executor>
  struct continuation {
    continuation(asio_pool in_self,
                 Executor *in_executor,
                 process_handler *in_handler)
        : executor_{in_executor},
          handler{in_handler},
          self_{in_self} {}

    template <typename Proc, typename... Args>
    continuation then(Args &&...args) {
      std::lock_guard l_g{self_->mutex_};
      //      static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
      auto proc = typename process_handler::instance_type{
          new Proc{std::forward<Args>(args)...},
          &asio_pool::deleter<Proc>};
      handler->next.reset(new process_handler{std::move(proc),
                                              &asio_pool::update<Proc>,
                                              &asio_pool::abort<Proc>,
                                              nullptr});
      handler = handler->next.get();
      return *this;
    }

    template <typename Func>
    continuation then(Func &&func) {
      return then<entt::process_adaptor<std::decay_t<Func>, Delta>>(std::forward<Func>(func));
    }

    Executor executor_;
    process_handler handler;

   private:
    asio_pool *self_;
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
  template <typename Proc, typename Executor>
  static continuation<Executor>
  post_asio(asio_pool *self, const Executor &in_executor, process_handler *handler) {
    boost::asio::post(
        boost::asio::bind_executor(
            in_executor,
            [in_proc = handler,
             self,
             in_executor]() {
              if (in_proc->update(*in_proc, {}, nullptr)) {
                std::lock_guard l_g{self->mutex_};
                self->handlers.erase(*in_proc);
              } else
                post_asio<Proc>(self, in_executor, in_proc);
            }));
    return continuation<Executor>{self, in_executor, handler};
  }

 private:
  handle_list handlers;
  std::recursive_mutex mutex_;

 public:
  /*! @brief Unsigned integer type. */
  using size_type                             = std::size_t;

  asio_pool()                                 = default;
  asio_pool(asio_pool &&) noexcept            = default;
  asio_pool &operator=(asio_pool &&) noexcept = default;

  template <typename Proc, typename Executor, typename... Args>
  auto attach(Executor in_executor, Args &&...args) {
    //    static_assert(std::is_base_of_v<entt::process<Proc, Delta>, Proc>, "Invalid process type");
    auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &asio_pool::deleter<Proc>};
    process_handler handler{std::move(proc), &asio_pool::update<Proc>, &asio_pool::abort<Proc>, nullptr};
    // forces the process to exit the uninitialized state
    // handler.update(handler, {}, nullptr);
    return post_asio<Executor>(this, in_executor, &handlers.emplace_back(std::move(handler)));
  }
};
}  // namespace doodle
