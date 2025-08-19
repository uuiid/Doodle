//
// Created by TD on 2024/2/27.
//

#include "json_body.h"

#include <doodle_core/exception/exception.h>
namespace doodle::http {
void basic_json_body::reader::init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
  if (length) {
    if (*length > json_str_.max_size()) {
      BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::buffer_overflow);
      return;
    }
    json_str_.reserve(boost::beast::detail::clamp(*length));
  }
  ec = {};
}

void basic_json_body::reader::finish(boost::system::error_code& ec) {
  if (json_type ::accept(json_str_)) {
    body_ = json_type::parse(json_str_);
    ec    = {};
  } else {
    BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::unexpected_body);
  }
}

void basic_json_body::writer::init(boost::system::error_code& ec) {
  json_str_ = body_.dump();
  ec        = {};
}

boost::optional<std::pair<basic_json_body::writer::const_buffers_type, bool>> basic_json_body::writer::get(
    boost::system::error_code& ec
) {
  ec = {};
  return {{const_buffers_type{json_str_.data(), json_str_.size()}, false}};
}
}  // namespace doodle::http