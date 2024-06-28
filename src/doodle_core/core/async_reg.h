#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::async_reg {

namespace detail {}

template <typename T, typename CompletionHandler>
auto async_get(const entt::registry& in_reg, const std::vector<entt::entity>& in_entts, CompletionHandler&& handler) {
  return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, std::vector<entt::entity>)>(
      [in_reg = &in_reg, in_entts = &in_entts](auto&& handler) {
        auto l_handler = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));
       },
      handler, &in_reg, std::move(in_entts)
  );
}
}  // namespace doodle::async_reg