//
// Created by td_main on 2023/8/17.
//
#include "url_route_put.h"

#include <doodle_lib/render_farm/detail/render_job_put.h>
namespace doodle::render_farm {
namespace detail {
void render_job_type_put::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto& l_session        = in_handle.get<working_machine_session>();
  using json_parser_type = boost::beast::http::request_parser<detail::basic_json_body>;
  auto l_parser_ptr      = std::make_shared<json_parser_type>(std::move(l_session.request_parser()));

  auto l_job_handle      = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi(in_cap.at("handle")))};
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      in_handle.emplace<render_job_put>(l_job_handle, l_parser_ptr)
  );
}
}  // namespace detail
}  // namespace doodle::render_farm