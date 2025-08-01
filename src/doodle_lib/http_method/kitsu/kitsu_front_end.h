//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {
class kitsu_front_end : public http_function {
 protected:
  std::shared_ptr<FSys::path> root_path_{};

 public:
  using http::http_function::http_function;
  ~kitsu_front_end() override = default;
  std::tuple<bool, std::shared_ptr<void>> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};

class get_files_kitsu_front_end : public kitsu_front_end {
 public:
  explicit get_files_kitsu_front_end(const std::shared_ptr<FSys::path>& in_root)
      : kitsu_front_end(boost::beast::http::verb::get) {
    root_path_ = in_root;
  }
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
};
class get_files_head_kitsu_front_end : public kitsu_front_end {
 public:
  explicit get_files_head_kitsu_front_end(const std::shared_ptr<FSys::path>& in_root)
      : kitsu_front_end(boost::beast::http::verb::head) {
    root_path_ = in_root;
  }
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
};

}  // namespace doodle::http