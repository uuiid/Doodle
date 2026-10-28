//
// Created by TD on 2025/10/26.
//

#include "http_body.h"

#include "doodle_core/exception/exception.h"

#include <doodle_lib/logger/logger.h>

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include "zlib_deflate_file_body.h"
#include <algorithm>
#include <core/global_function.h>
#include <cryptopp/adler32.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <utility>

namespace doodle::http {

std::string http_body_base_t::generate_etag(const FSys::path& in_path) {
  auto l_time = FSys::file_time_type::clock::to_utc(FSys::last_write_time(in_path));
  std::string l_path_adler{};
  {
    CryptoPP::Adler32 l_adler;
    CryptoPP::StringSource l_string_source{
        in_path.generic_string(), true,
        new CryptoPP::HashFilter(l_adler, new CryptoPP::HexEncoder{new CryptoPP::StringSink{l_path_adler}})
    };
  }
  return fmt::format("\"{}-{}-{}\"", l_time.time_since_epoch().count(), FSys::file_size(in_path), l_path_adler);
}

std::string http_body_base_t::zlib_compress(const std::string& in_str) {
  std::stringstream compressed{};
  std::stringstream original{in_str};
  boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
  out.push(boost::iostreams::zlib_compressor{});
  out.push(original);
  boost::iostreams::copy(out, compressed);
  return compressed.str();
}

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
http_body_base_t& http_body_base_t::set_header(boost::beast::http::field in_field, std::string in_value) {
  headers_.set(in_field, std::move(in_value));
  return *this;
}
http_body_base_t& http_body_base_t::add_header(boost::beast::http::field in_field, std::string in_value) {
  headers_.insert(in_field, std::move(in_value));
  return *this;
}

boost::beast::http::status http_body_base_t::get_status() const { return status_; }
std::string_view http_body_base_t::get_mine_type() const { return mine_type_; }
bool http_body_base_t::has_cache_control() const { return has_cache_control_; }
bool http_body_base_t::is_deflate() const { return is_deflate_; }
bool http_body_base_t::is_attachment() const { return is_attachment_; }
const std::string& http_body_base_t::get_attachment_filename() const { return attachment_filename_; }
const boost::beast::http::response_header<>& http_body_base_t::get_headers() const { return headers_; }

boost::beast::http::status http_body_base_t::body_status(const detail::session_data& in_session_data) const {
  return status_ == boost::beast::http::status::ok && in_session_data.method() == boost::beast::http::verb::post
             ? boost::beast::http::status::created
             : status_;
}

http_body_json_t::http_body_json_t(nlohmann::json in_body, boost::beast::http::status in_status)
    : http_body_base_t(in_status, "application/json; charset=utf-8"), body_(std::move(in_body)) {}

boost::beast::http::message_generator http_body_json_t::make_message(
    const detail::session_data& in_session_data
) const {
  boost::beast::http::response<boost::beast::http::string_body> l_res{
      body_status(in_session_data), in_session_data.version_
  };
  for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
  set_response_header(l_res, get_mine_type(), in_session_data.access_control_allow_origin_, in_session_data.timeout_);
  l_res.keep_alive(in_session_data.keep_alive_);
  l_res.body() = body_.dump();
  if (is_deflate() && in_session_data.is_deflate()) {
    l_res.set(boost::beast::http::field::content_encoding, "deflate");
    l_res.body() = zlib_compress(l_res.body());
  }
  l_res.prepare_payload();
  return l_res;
}

http_body_string_t::http_body_string_t(
    std::string in_body, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type), body_(std::move(in_body)) {}

boost::beast::http::message_generator http_body_string_t::make_message(
    const detail::session_data& in_session_data
) const {
  boost::beast::http::response<boost::beast::http::string_body> l_res{
      body_status(in_session_data), in_session_data.version_
  };
  for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
  set_response_header(l_res, get_mine_type(), in_session_data.access_control_allow_origin_, in_session_data.timeout_);
  l_res.keep_alive(in_session_data.keep_alive_);
  l_res.body() = body_;
  if (is_deflate() && in_session_data.is_deflate()) {
    l_res.set(boost::beast::http::field::content_encoding, "deflate");
    l_res.body() = zlib_compress(l_res.body());
  }
  l_res.prepare_payload();
  return l_res;
}

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

boost::beast::http::message_generator http_body_binary_t::make_message(
    const detail::session_data& in_session_data
) const {
  boost::beast::http::response<boost::beast::http::vector_body<unsigned char>> l_res{
      get_status(), in_session_data.version_
  };
  for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
  set_response_header(l_res, get_mine_type(), in_session_data.access_control_allow_origin_, in_session_data.timeout_);
  l_res.keep_alive(in_session_data.keep_alive_);
  l_res.body().resize(body_.size());
  for (std::size_t l_i = 0; l_i < body_.size(); ++l_i) l_res.body()[l_i] = static_cast<unsigned char>(body_[l_i]);
  l_res.prepare_payload();
  return l_res;
}

http_body_file_t::http_body_file_t(
    FSys::path in_path, std::string_view in_mime_type, boost::beast::http::status in_status
)
    : http_body_base_t(in_status, in_mime_type), path_(std::move(in_path)) {
  DOODLE_CHICK_HTTP(FSys::exists(path_), not_found, "文件不存在 {}", path_);
}

http_body_file_t& http_body_file_t::set_range_mode(http_file_range_mode in_mode) {
  range_mode_ = in_mode;
  return *this;
}
http_body_file_t& http_body_file_t::set_range(std::uint64_t in_begin, std::uint64_t in_end) {
  range_mode_  = http_file_range_mode::partial;
  range_begin_ = in_begin;
  range_end_   = in_end;
  return *this;
}
http_body_file_t& http_body_file_t::set_deflate_body(bool in_deflate) {
  use_deflate_body_ = in_deflate;
  return *this;
}

boost::beast::http::message_generator http_body_file_t::make_message(
    const detail::session_data& in_session_data
) const {
  if (!FSys::exists(path_))
    return http_body_error_t{boost::beast::http::status::not_found, "文件不存在"}.make_message(in_session_data);

  if (range_mode_ == http_file_range_mode::head_only || in_session_data.method() == boost::beast::http::verb::head) {
    boost::beast::http::response<boost::beast::http::empty_body> l_res{
        boost::beast::http::status::ok, in_session_data.version_
    };
    for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
    set_response_file_header(
        l_res, path_, get_mine_type(), has_cache_control(), is_attachment(), get_attachment_filename(),
        in_session_data.access_control_allow_origin_, in_session_data.timeout_
    );
    l_res.keep_alive(in_session_data.keep_alive_);
    l_res.prepare_payload();
    return l_res;
  }

  boost::system::error_code l_code{};
  if (use_deflate_body_) {
    boost::beast::http::response<zlib_deflate_file_body> l_res{
        boost::beast::http::status::ok, in_session_data.version_
    };
    for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
    l_res.body().open(path_, std::ios::in | std::ios::binary, l_code);
    if (l_code)
      throw_exception(
          http_request_error{
              boost::beast::http::status::internal_server_error,
              fmt::format("无法打开文件 {} {}", path_.generic_string(), l_code.message())
          }
      );
    l_res.set(boost::beast::http::field::content_encoding, "deflate");
    set_response_file_header(
        l_res, path_, get_mine_type(), has_cache_control(), is_attachment(), get_attachment_filename(),
        in_session_data.access_control_allow_origin_, in_session_data.timeout_
    );
    l_res.keep_alive(in_session_data.keep_alive_);
    l_res.prepare_payload();
    return l_res;
  }

  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_session_data.version_
  };
  for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
  l_res.body().open(path_.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  if (l_code)
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format("无法打开文件 {} {}", path_.generic_string(), l_code.message())
        }
    );

  auto l_begin = range_begin_;
  if (range_mode_ == http_file_range_mode::partial) {
    l_res.body().seek(l_begin, l_code);
    if (l_code)
      throw_exception(
          http_request_error{
              boost::beast::http::status::internal_server_error,
              fmt::format("无法定位文件 {} {}", path_.generic_string(), l_code.message())
          }
      );
    l_res.result(boost::beast::http::status::partial_content);
    l_res.set(
        boost::beast::http::field::content_range,
        fmt::format("bytes {}-{}/{}", l_begin, FSys::file_size(path_) - 1, FSys::file_size(path_))
    );
  } else if (in_session_data.req_header_.find(boost::beast::http::field::range) != in_session_data.req_header_.end()) {
    if (auto l_range = in_session_data.req_header_.at(boost::beast::http::field::range);
        !l_range.empty() && l_range.starts_with("bytes=")) {
      l_begin = std::stoll(l_range.substr(6, l_range.find("-") - 6));
      l_res.body().seek(l_begin, l_code);
      if (l_code)
        throw_exception(
            http_request_error{
                boost::beast::http::status::internal_server_error,
                fmt::format("无法定位文件 {} {}", path_.generic_string(), l_code.message())
            }
        );
      l_res.result(boost::beast::http::status::partial_content);
      l_res.set(
          boost::beast::http::field::content_range,
          fmt::format("bytes {}-{}/{}", l_begin, FSys::file_size(path_) - 1, FSys::file_size(path_))
      );
    }
  }

  set_response_file_header(
      l_res, path_, get_mine_type(), has_cache_control(), is_attachment(), get_attachment_filename(),
      in_session_data.access_control_allow_origin_, in_session_data.timeout_
  );
  l_res.keep_alive(in_session_data.keep_alive_);
  l_res.prepare_payload();
  return l_res;
}

http_body_empty_t::http_body_empty_t()
    : http_body_base_t(boost::beast::http::status::no_content, "application/json; charset=utf-8") {}

http_body_empty_t::http_body_empty_t(boost::beast::http::status in_status)
    : http_body_base_t(in_status, "application/json; charset=utf-8") {}

boost::beast::http::message_generator http_body_empty_t::make_message(
    const detail::session_data& in_session_data
) const {
  boost::beast::http::response<boost::beast::http::empty_body> l_res{get_status(), in_session_data.version_};
  for (auto const& l_field : headers_) l_res.insert(l_field.name(), l_field.value());
  set_response_header(l_res, get_mine_type(), in_session_data.access_control_allow_origin_, in_session_data.timeout_);
  l_res.keep_alive(in_session_data.keep_alive_);
  l_res.prepare_payload();
  return l_res;
}

http_body_error_t::http_body_error_t(boost::beast::http::status in_status, std::string in_message, std::int32_t in_code)
    : http_body_base_t(in_status, "application/json; charset=utf-8"), message_(std::move(in_message)), code_(in_code) {}

boost::beast::http::message_generator http_body_error_t::make_message(
    const detail::session_data& in_session_data
) const {
  in_session_data.logger_->log(log_loc(), level::err, "发送错误码 {} {}", get_status(), message_);

  auto l_msg_code = code_ == -1 ? enum_to_num(get_status()) : code_;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      get_status(), in_session_data.version_
  };
  for (auto const& l_field : headers_) l_response.insert(l_field.name(), l_field.value());
  set_response_header(
      l_response, get_mine_type(), in_session_data.access_control_allow_origin_, in_session_data.timeout_
  );
  l_response.keep_alive(in_session_data.keep_alive_);
  l_response.body() = nlohmann::json{{"error", message_}, {"code", l_msg_code}}.dump();
  l_response.prepare_payload();
  return l_response;
}

}  // namespace doodle::http
