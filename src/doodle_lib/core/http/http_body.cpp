//
// Created by TD on 2025/10/26.
//

#include "http_body.h"

#include <utility>

namespace doodle::http {

http_body_base_t::http_body_base_t(boost::beast::http::status in_status, std::string_view in_mine_type)
    : status_(in_status), mine_type_(in_mine_type) {}

http_body_base_t& http_body_base_t::set_status(boost::beast::http::status in_status) {
  status_ = in_status;
  return *this;
}
http_body_base_t& http_body_base_t::set_mine_type(std::string_view in_mine_type) {
  mine_type_ = in_mine_type;
  return *this;
}
http_body_base_t& http_body_base_t::set_deflate(bool in_deflate) {
  is_deflate_ = in_deflate;
  return *this;
}
http_body_base_t& http_body_base_t::set_cache_control(bool in_enable) {
  has_cache_control_ = in_enable;
  return *this;
}
http_body_base_t& http_body_base_t::set_attachment(std::string in_filename) {
  is_attachment_       = true;
  attachment_filename_ = std::move(in_filename);
  return *this;
}

boost::beast::http::status http_body_base_t::get_status() const { return status_; }
std::string_view http_body_base_t::get_mine_type() const { return mine_type_; }
bool http_body_base_t::has_cache_control() const { return has_cache_control_; }
bool http_body_base_t::is_deflate() const { return is_deflate_; }
bool http_body_base_t::is_attachment() const { return is_attachment_; }
const std::string& http_body_base_t::get_attachment_filename() const { return attachment_filename_; }

http_body_json_t::http_body_json_t(nlohmann::json in_body, boost::beast::http::status in_status)
    : http_body_base_t(in_status, "application/json; charset=utf-8"), body_(std::move(in_body)) {}

http_body_string_t::http_body_string_t(
    std::string in_body, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type), body_(std::move(in_body)) {}

http_body_binary_t::http_body_binary_t(
    std::vector<std::byte> in_body, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type), body_(std::move(in_body)) {}

http_body_binary_t::http_body_binary_t(
    std::vector<unsigned char> in_body, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type) {
  body_.resize(in_body.size());
  for (std::size_t l_i = 0; l_i < in_body.size(); ++l_i) body_[l_i] = static_cast<std::byte>(in_body[l_i]);
}

http_body_file_t::http_body_file_t(
    FSys::path in_path, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type), path_(std::move(in_path)) {}

http_body_empty_t::http_body_empty_t()
    : http_body_base_t(boost::beast::http::status::no_content, "application/json; charset=utf-8") {}

http_body_error_t::http_body_error_t(
    boost::beast::http::status in_status, std::string in_message, std::int32_t in_code
)
    : http_body_base_t(in_status, "application/json; charset=utf-8"),
      message_(std::move(in_message)),
      code_(in_code) {}

}  // namespace doodle::http
