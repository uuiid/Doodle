#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/hf_tokenizer.h>
#include <doodle_lib/ai/llm2vec.h>
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

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_post_args& out) {
    if (in_json.contains("text") && in_json.at("text").is_string()) in_json.at("text").get_to(out.text_);
  }
};

}  // namespace

struct ai_train_animation::impl {
  std::shared_ptr<LLM2Vec> model_{};
  impl() = default;
  std::once_flag init_flag_;
  void init() {
    model_ = std::make_shared<LLM2Vec>(
        R"(D:\ai_mod\onnx-McGill-NLP--LLM2Vec-Meta-Llama-3-8B-Instruct-mntp\model.onnx)",
        R"(D:\ai_mod\onnx-McGill-NLP--LLM2Vec-Meta-Llama-3-8B-Instruct-mntp\tokenizer.json)"
    );
  }
  void run(const std::string& text) {
    std::call_once(init_flag_, &impl::init, this);
    auto embedding = (*model_)("", text);
    SPDLOG_INFO(
        "Generated embedding of size {} for input text '{}' [{}]", embedding.size(), text, fmt::join(embedding, ",")
    );
  }
};
ai_train_animation::ai_train_animation() : impl_ptr_(std::make_shared<impl>()) {}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
#ifndef NDEBUG
  init_ort_env();
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  boost::asio::post(g_io_context(), [this, l_args]() { impl_ptr_->run(l_args.text_); });
#endif
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http