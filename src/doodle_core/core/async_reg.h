#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entt.hpp>
namespace doodle::async_reg {

namespace detail {

template <typename T, typename CompletionHandler>
class async_get_op
    : public boost::asio::coroutine,
      public boost::beast::async_base<CompletionHandler, boost::asio::strand<boost::asio::io_context::executor_type>> {
 public:
  using base_type =
      boost::beast::async_base<CompletionHandler, boost::asio::strand<boost::asio::io_context::executor_type>>;
  async_get_op(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler)
      : base_type{std::forward(handler), g_strand()}, reg_{in_reg}, entts_{in_entts} {}

  void operator()() {
    std::vector<std::optional<T>> l_result;
    BOOST_ASIO_CORO_REENTER(*this) {
      for (const auto& l_entt : entts_) {
        if (reg_.valid(l_entt) && reg_.all_of<T>(l_entt))
          l_result.emplace_back(reg_.get<T>(l_entt));
        else
          l_result.emplace_back(std::nullopt);
      }
      this->complete(boost::system::error_code{}, std::move(l_result));
    }
  }

 private:
  const entt::registry& reg_;
  const std::vector<entt::entity>& entts_;
};

}  // namespace detail

template <typename T, typename CompletionHandler>
auto async_get(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, std::vector<std::optional<T>>)>(
      [in_reg = &in_reg, in_entts = &in_entts](auto&& handler) {
        auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
      },
      handler, &in_reg, std::move(in_entts)
  );
}
}  // namespace doodle::async_reg