#include <doodle_lib/ai/bone_weight_inference.h>

#include "ai_main.h"
#include <filesystem>
#include <vector>

namespace doodle::http {

struct ai_train_binding_weights_args {
  std::vector<FSys::path> input_path_{};
  FSys::path output_path_{};

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_args& out) {
    in_json.at("input_path").get_to(out.input_path_);
    in_json.at("output_path").get_to(out.output_path_);
  }
};
boost::asio::awaitable<boost::beast::http::message_generator> ai_train_binding_weights::post(
    session_data_ptr in_handle
) {
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_args>();
  ai::bone_weight_inference_model::train(l_args.input_path_, l_args.output_path_);
  co_return in_handle->make_msg(nlohmann::json{});
}
}  // namespace doodle::http