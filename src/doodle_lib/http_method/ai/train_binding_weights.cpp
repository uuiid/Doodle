#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/kimodo.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/http_method/ai/ai_main.h>

#include <boost/asio/post.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>
#include <string>

namespace doodle::http {
namespace {
// 初始化 onnxruntime 环境
void _init_ort_env() {
  try {
    auto env                         = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "doodle_ort");
    core_set::get_set().ort_env_ptr_ = env;
    SPDLOG_INFO("ONNX Runtime 环境初始化成功");
  } catch (const Ort::Exception& e) {
    SPDLOG_ERROR("ONNX Runtime 环境初始化失败: {}", e.what());
  }
}
void init_ort_env() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, &_init_ort_env);
}

struct ai_train_binding_weights_post_args {
  std::string text_{};
  std::int32_t num_frames_{120};

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_post_args& out) {
    if (in_json.contains("text") && in_json.at("text").is_string()) in_json.at("text").get_to(out.text_);
    if (in_json.contains("num_frames") && in_json.at("num_frames").is_number_integer())
      in_json.at("num_frames").get_to(out.num_frames_);
  }
};

}  // namespace

struct ai_train_animation::impl {
  std::shared_ptr<doodle::ai::kimodo> model_{};
  impl() = default;
  std::once_flag init_flag_;
  void init() {
    model_        = std::make_shared<doodle::ai::kimodo>();
    auto l_config = std::make_shared<doodle::ai::kimodo_model_config>();
    l_config->load_from_json(R"(D:\ai_mod\onnx-models--nvidia--Kimodo-SOMA-RP-v1.1)");
    model_->load(l_config);
  }
  void run(const ai_train_binding_weights_post_args& in_args) {
    std::call_once(init_flag_, &impl::init, this);
    auto output = model_->generate(in_args.text_, in_args.num_frames_, 50);
  }
};
ai_train_animation::ai_train_animation() : impl_ptr_(std::make_shared<impl>()) {}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
#ifndef NDEBUG
  init_ort_env();
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  boost::asio::post(g_io_context(), [this, l_args]() { impl_ptr_->run(l_args); });
#endif
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http