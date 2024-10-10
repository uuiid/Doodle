//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::kitsu {
class kitsu_front_end : public doodle::http::http_function_base_t {
 public:
  std::shared_ptr<FSys::path> root_path_{};
  explicit kitsu_front_end(
      FSys::path&& in_root, boost::beast::http::verb in_verb,
      std::function<boost::asio::awaitable<boost::beast::http::message_generator>(http::session_data_ptr)> in_callback
  )
      : http::http_function_base_t(in_verb, std::move(in_callback), nullptr),
        root_path_(std::make_shared<FSys::path>(std::move(in_root))) {}
  ~kitsu_front_end() override = default;
  std::tuple<bool, http::capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
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
  explicit kitsu_proxy_url(
      const std::string& in_url
  )
      : http::http_function_base_t(), url_segments_(split_url(in_url)) {}
  ~kitsu_proxy_url() override = default;
  std::tuple<bool, http::capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};

}  // namespace doodle::kitsu