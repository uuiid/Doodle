#include "doodle_core/core/global_function.h"

#include <doodle_lib/ai/bone_weight_inference.h>

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
struct ai_train_binding_weights_put_args {
  FSys::path model_path_{};
  FSys::path fbx_path_{};

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_put_args& out) {
    in_json.at("model_path").get_to(out.model_path_);
    in_json.at("fbx_path").get_to(out.fbx_path_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> ai_train_binding_weights::post(
    session_data_ptr in_handle
) {
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  boost::asio::post(g_io_context(), [l_args]() {
    doodle::ai::bone_weight_inference_model::train(l_args.input_path_, l_args.output_path_);
  });
  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> ai_train_binding_weights::put(
    session_data_ptr in_handle
) {
  auto l_args     = in_handle->get_json().get<ai_train_binding_weights_put_args>();
  auto l_out_path = l_args.fbx_path_.parent_path() / (l_args.fbx_path_.stem().string() + "_bound.fbx");
  boost::asio::post(g_io_context(), [l_args, l_out_path]() {
    doodle::ai::bone_weight_inference_model l_model{l_args.model_path_};
    SPDLOG_WARN("开始推理模型 {} {} {}", l_args.model_path_, l_args.fbx_path_, l_out_path);
    l_model.predict_by_fbx(l_args.fbx_path_, l_out_path);
  });

  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http