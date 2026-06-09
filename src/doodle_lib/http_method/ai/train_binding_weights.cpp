#include <doodle_lib/core/global_function.h>

#include <boost/asio/post.hpp>

#include "ai_main.h"
#include <filesystem>
#include <spdlog/spdlog.h>
#include <vector>

namespace doodle::http {
namespace {
struct ai_train_binding_weights_post_args {
  std::vector<FSys::path> input_path_{};
  FSys::path output_path_{};

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_post_args& out) {
    in_json.at("input_path").get_to(out.input_path_);
    in_json.at("output_path").get_to(out.output_path_);
  }
};

}  // namespace
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  boost::asio::post(g_io_context(), [l_args]() {
#ifndef NDEBUG

#endif
  });
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http