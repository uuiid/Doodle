//
// Created by TD on 24-7-19.
//

#pragma once
#include <boost/asio.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <queue>

namespace doodle {
class awaitable_queue_limitation {
 public:
  class queue_guard;
  using queue_guard_ptr = std::shared_ptr<queue_guard>;

 private:
  struct awaitable_queue_impl;

  template <typename CompletionToken>
  struct call_fun_t {
    boost::asio::any_io_executor executor_{};
    std::shared_ptr<std::decay_t<CompletionToken>> handler_;
    awaitable_queue_limitation* queue_{};

    void operator()() const {
      boost::asio::post(
          boost::asio::bind_executor(
              executor_, boost::asio::prepend(std::move(*handler_), std::make_shared<queue_guard>(*queue_))
          )
      );
    }
  };

 public:
  awaitable_queue_limitation() = default;

  explicit awaitable_queue_limitation(const std::int32_t in_limit) : impl_{std::make_shared<awaitable_queue_impl>()} {
    impl_->limit_ = in_limit;
  }

  ~awaitable_queue_limitation() = default;

  class queue_guard {
    std::shared_ptr<awaitable_queue_impl> impl_;

   public:
    explicit queue_guard(awaitable_queue_limitation& in_queue) : impl_{in_queue.impl_} {
      ++impl_->run_task_;
    }

    ~queue_guard() {
      --impl_->run_task_;
      impl_->maybe_invoke();
    }
  };

  template <typename CompletionToken>
  auto queue(CompletionToken&& in_token = boost::asio::use_awaitable) {
    boost::asio::any_io_executor l_exe = boost::asio::get_associated_executor(in_token);
    return boost::asio::async_initiate<CompletionToken, void(queue_guard_ptr)>(
        [](auto&& in_compl, awaitable_queue_limitation* in_self, const boost::asio::any_io_executor& in_exe) {
          // if (in_self->impl_->run_task_ == 0) {
          //   ++in_self->impl_->run_task_;
          //   boost::asio::dispatch(boost::asio::bind_executor(
          //       in_exe, boost::asio::prepend(std::move(in_compl), std::make_shared<queue_guard>(*in_self))
          //   ));
          //   return;
          // }

          call_fun_t<std::decay_t<decltype(in_compl)>> l_fun{
              in_exe, std::make_shared<std::decay_t<decltype(in_compl)>>(std::forward<decltype(in_compl)>(in_compl)),
              in_self
          };
          in_self->impl_->await_suspend(l_fun);
          in_self->impl_->maybe_invoke();
        },
        in_token, this, l_exe
    );
  }

  void set_limit(std::int32_t in_limit) { impl_->limit_ = in_limit; }

 private:
  struct awaitable_queue_impl {
    std::queue<std::function<void()>> next_list_{};
    std::recursive_mutex lock_;
    std::atomic_int limit_{1};
    std::atomic_int run_task_;

    awaitable_queue_impl()  = default;

    ~awaitable_queue_impl() = default;
    void await_suspend(std::function<void()> in_handle);

    bool next();
    void maybe_invoke();
  };

  std::shared_ptr<awaitable_queue_impl> impl_ = std::make_shared<awaitable_queue_impl>();
};
}  // namespace doodle