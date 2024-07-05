#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entt.hpp>
namespace doodle::async_reg {

namespace detail {

template <typename T, typename CompletionHandler>
class async_get_op : public boost::asio::coroutine,
                     public boost::beast::async_base<CompletionHandler, boost::asio::any_io_executor> {
 public:
  using base_type = boost::beast::async_base<CompletionHandler, boost::asio::any_io_executor>;
  async_get_op(
      const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, boost::asio::any_io_executor in_exe,
      CompletionHandler&& in_handler
  )
      : base_type{std::forward(in_handler), std::move(in_exe)}, reg_{in_reg}, entts_{in_entts} {
    (*this)();
  }

  void operator()() {
    std::vector<std::optional<T>> l_result;
    // 切换到 g_strand 线程
    co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
    for (const auto& l_entt : entts_) {
      if (reg_.valid(l_entt) && reg_.all_of<T>(l_entt))
        l_result.emplace_back(reg_.get<T>(l_entt));
      else
        l_result.emplace_back(std::nullopt);
    }
    this->complete(boost::system::error_code{}, std::move(l_result));
  }

 private:
  const entt::registry& reg_;
  const std::vector<entt::entity>& entts_;
};

}  // namespace detail

template <typename T, typename CompletionHandler>
auto async_get(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  auto l_exe = boost::asio::get_associated_executor(handler, g_io_context());
  return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, std::vector<std::optional<T>>)>(
      [l_exe](auto&& handler, const entt::registry& in_reg, const std::vector<entt::entity>& in_entts) {
        detail::async_get_op<T, std::decay_t<decltype(handler)>> l_op{
            in_reg, in_entts, l_exe, std::forward<decltype(handler)>(handler)
        };
      },
      handler, in_reg, in_entts
  );
}
}  // namespace doodle::async_reg