//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {
class kitsu_front_end : public http_function_base_t {
 protected:
  std::shared_ptr<FSys::path> root_path_{};

 public:
  using http::http_function_base_t::http_function_base_t;
  ~kitsu_front_end() override = default;
  std::tuple<bool, std::shared_ptr<void>> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};

class kitsu_proxy_url : public doodle::http::http_function_base_t {
  std::vector<std::string> url_segments_{};
  static std::vector<std::string> split_url(const std::string& in_url) {
    std::vector<std::string> l_result;
    boost::split(l_result, in_url, boost::is_any_of("/"));
    std::erase_if(l_result, [](const std::string& in_str) { return in_str.empty(); });
    return l_result;
  }

 public:
  explicit kitsu_proxy_url(const std::string& in_url)
      : http::http_function_base_t(), url_segments_(split_url(in_url)) {}
  ~kitsu_proxy_url() override = default;
  std::tuple<bool, std::shared_ptr<void>> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
  [[nodiscard]] bool is_proxy() const override;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
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