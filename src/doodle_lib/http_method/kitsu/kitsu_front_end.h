//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::kitsu {
class kitsu_front_end : public doodle::http::http_function_base_t {
 protected:
  FSys::path root_path_{};
  virtual boost::asio::awaitable<boost::beast::http::message_generator> get_files(http::session_data_ptr) const;

 public:
  explicit kitsu_front_end(FSys::path&& in_root)
      : http::http_function_base_t(boost::beast::http::verb::get, std::bind_front(&get_files, this), nullptr),
        root_path_(std::move(in_root)) {}
  ~kitsu_front_end() override = default;
  std::tuple<bool, http::capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};
class kitsu_front_end_head : public kitsu_front_end {
 protected:
  boost::asio::awaitable<boost::beast::http::message_generator> get_files(http::session_data_ptr) const override;

 public:
  explicit kitsu_front_end_head(FSys::path&& in_root) : kitsu_front_end(std::move(in_root)) {
    verb_ = boost::beast::http::verb::head;
  }
};
}  // namespace doodle::kitsu