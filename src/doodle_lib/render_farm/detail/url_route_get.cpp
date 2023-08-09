//
// Created by td_main on 2023/8/9.
//

#include "url_route_get.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>
namespace doodle::render_farm::detail {
boost::beast::http::message_generator url_rote_get::run(
    const std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>>& in_request_parser
) {
  get_uuiid();
  if (call_back_map_.count(method_) == 0) {
    throw_exception(doodle_error{"not find method"});
  }
  auto l_url = boost::url{in_request_parser->get().target()};
  return call_back_map_.at(method_)(l_url.params(), handle_);
}
void url_rote_get::get_uuiid() {
  auto l_begin = segments_ref_.begin();
  auto l_end   = segments_ref_.end();

  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found id"});
  }
  ++l_begin;
  ++l_begin;

  auto l_uuid = boost::lexical_cast<boost::uuids::uuid>(*l_begin);
  g_reg()->view<boost::uuids::uuid>().each([&](entt::entity in_entity, boost::uuids::uuid& in_uuid) {
    if (in_uuid == l_uuid) {
      handle_ = entt::handle{*g_reg(), in_entity};
    }
  });
  if (!handle_) {
    throw_exception(doodle_error{"url not found id"});
  }

  ++l_begin;
  if (l_begin != l_end) {
    throw_exception(doodle_error{"url not found method"});
  }

  method_ = *l_begin;
}

}  // namespace doodle::render_farm::detail