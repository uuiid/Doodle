//
// Created by TD on 2024/2/21.
//

#include <doodle_core/doodle_core_fwd.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_function;

using http_function_ptr = std::shared_ptr<http_function>;
class http_route {
 private:
  using map_actin_type = std::vector<http_function_ptr>;
  std::map<boost::beast::http::verb, map_actin_type> actions;

 public:
  // 注册路由
  http_route& reg(const http_function_ptr in_function);
  // 路由分发
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const entt::handle& in_handle
  ) const;
};
}  // namespace doodle::http