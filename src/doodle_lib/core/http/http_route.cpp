//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {

http_route& http_route::reg(const doodle::http::http_function_ptr in_function) {
  actions[in_function->get_verb()].emplace_back(in_function);
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const entt::handle& in_handle
) const {
  auto l_iter = actions.find(in_verb);
  if (l_iter == actions.end()) return nullptr;
  for (const auto& i : l_iter->second) {
    if (auto&& [l_m, l_cat] = i->set_match_url(in_segment); l_m) {
      in_handle.emplace_or_replace<http_function::capture_t>(l_cat);
      return i;
    }
  }
  return nullptr;
}

}  // namespace doodle::http