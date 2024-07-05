#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entt.hpp>
namespace doodle::async_reg {

namespace detail {

// template <typename T, typename CompletionHandler>
// class async_get_op : public boost::asio::coroutine,
//                      public boost::beast::async_base<CompletionHandler, boost::asio::any_io_executor> {
//  public:
//   using base_type = boost::beast::async_base<CompletionHandler, boost::asio::any_io_executor>;
//   async_get_op(
//       const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, boost::asio::any_io_executor in_exe,
//       CompletionHandler&& in_handler
//   )
//       : base_type{std::forward(in_handler), std::move(in_exe)}, reg_{in_reg}, entts_{in_entts} {
//     (*this)();
//   }

//   void operator()() {
//     // 切换到 g_strand 线程
//     std::vector<std::optional<T>> l_result;
//     co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
//     for (const auto& l_entt : entts_) {
//       if (reg_.valid(l_entt) && reg_.all_of<T>(l_entt))
//         l_result.emplace_back(reg_.get<T>(l_entt));
//       else
//         l_result.emplace_back(std::nullopt);
//     }
//     this->complete(boost::system::error_code{}, std::move(l_result));
//   }

//  private:
//   const entt::registry& reg_;
//   const std::vector<entt::entity>& entts_;
// };

}  // namespace detail

template <typename T, typename CompletionHandler>
auto async_get(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void(std::vector<std::optional<T>>)>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const std::vector<entt::entity>& in_entts,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        std::vector<std::optional<T>> l_result;
        for (const auto& l_entt : in_entts) {
          if (reg_.valid(l_entt) && reg_.all_of<T>(l_entt))
            l_result.emplace_back(reg_.get<T>(l_entt));
          else
            l_result.emplace_back(std::nullopt);
        }
        boost::asio::post(l_exe, std::bind_front(handler, l_result));
      },
      handler, in_reg, in_entts, l_exe
  );
}
template <typename T, typename CompletionHandler>
auto async_get(const entt::registry& in_reg, entt::entity& in_entt, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void(std::optional < T >>)>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const entt::entity& in_entt,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        std::optional<T> l_result{};
        if (reg_.valid(in_entt) && reg_.all_of<T>(in_entt)) l_result = reg_.get<T>(in_entt);

        boost::asio::post(l_exe, std::bind_front(handler, l_result));
      },
      handler, in_reg, in_entt, l_exe
  );
}

template <typename T, typename CompletionHandler>
auto async_valid(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void(std::vector<bool>)>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const std::vector<entt::entity>& in_entts,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        std::vector<bool> l_result;
        for (const auto& l_entt : in_entts) {
          l_result.emplace_back(in_reg.valid(l_entt) && in_reg.all_of<T>(l_entt));
        }
        boost::asio::post(in_exe, std::bind_front(handler, l_result));
      },
      handler, in_reg, in_entts, l_exe
  );
}
template <typename T, typename CompletionHandler>
auto async_valid(const entt::registry& in_reg, const entt::entity& in_entt, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void(bool)>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const entt::entity& in_entt,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        bool l_result = in_reg.valid(in_entt) && in_reg.all_of<T>(in_entt);
        boost::asio::post(in_exe, std::bind_front(handler, l_result));
      },
      handler, in_reg, in_entt, l_exe
  );
}

template <typename T, typename CompletionHandler>
auto async_emplace_or_replace(
    entt::registry& in_reg, const std::vector<std::tuple<entt::entity, T>>& in_entts, CompletionHandler&& handler
) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, entt::registry& in_reg, const std::vector<std::tuple<entt::entity, T>>& in_entts,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        for (const auto& [l_entt, l_value] : in_entts) {
          if (in_reg.valid(l_entt)) {
            in_reg.emplace_or_replace<T>(l_entt, std::forward<T>(l_value));
          } else {
          }
        }
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entts, l_exe
  );
}
template <typename T, typename CompletionHandler>
auto async_emplace_or_replace(
    entt::registry& in_reg, const std::tuple<entt::entity, T>& in_entt, CompletionHandler&& handler
) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, entt::registry& in_reg, const std::tuple<entt::entity, T>& in_entt,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        const auto& [l_entt, l_value] = in_entt;
        if (in_reg.valid(l_entt)) {
          in_reg.emplace_or_replace<T>(l_entt, std::forward<T>(l_value));
        } else {
        }
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entt, l_exe
  );
}

template <typename CompletionHandler>
auto async_destroy(
    const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler
) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const std::vector<entt::entity>& in_entts,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        for (const auto& l_entt : in_entts) {
          if (in_reg.valid(l_entt)) in_reg.destroy(l_entt);
        }
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entts, l_exe
  );
}
template <typename CompletionHandler>
auto async_destroy(const entt::registry& in_reg, const entt::entity& in_entt, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, const entt::registry& in_reg, const entt::entity& in_entt,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        if (in_reg.valid(in_entt)) in_reg.destroy(in_entt);
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entt, l_exe
  );
}

template <typename T, typename CompletionHandler>
auto async_remove(entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, entt::registry& in_reg, const std::vector<entt::entity>& in_entts,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        for (const auto& l_entt : in_entts) {
          if (in_reg.valid(l_entt)) in_reg.remove<T>(l_entt);
        }
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entts, l_exe
  );
}
template <typename T, typename CompletionHandler>
auto async_remove(entt::registry& in_reg, const entt::entity& in_entt, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void()>(
      // warning: 此处协程不一定急切执行,  不可使用 lambda 捕获, 会使堆栈错误
      [](auto&& handler, entt::registry& in_reg, const entt::entity& in_entt,
         const boost::asio::any_io_executor& in_exe) {
        // 切换到 g_strand 线程
        co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
        if (in_reg.valid(in_entt)) in_reg.remove<T>(in_entt);
        boost::asio::post(in_exe, handler);
      },
      handler, in_reg, in_entt, l_exe
  );
}
}  // namespace doodle::async_reg