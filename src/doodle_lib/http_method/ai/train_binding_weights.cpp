#include <doodle_lib/core/global_function.h>

#include <boost/asio/post.hpp>

#include "ai_main.h"
#include <filesystem>
#include <spdlog/spdlog.h>
#include <tokenizers_cpp.h>
#include <vector>

namespace doodle::http {
namespace {
// 运行分词器
void run_tokenizer(const std::string& in_text) {
  FSys::ifstream l_file("D:\\ai_mod\\onnx-McGill-NLP--LLM2Vec-Meta-Llama-3-8B-Instruct-mntp\\tokenizer.json");
  auto l_json_str = std::string{std::istreambuf_iterator<char>(l_file), std::istreambuf_iterator<char>()};
  auto tokenizer  = tokenizers::Tokenizer::FromBlobJSON(l_json_str);
  auto output     = tokenizer->Encode(in_text);
  SPDLOG_INFO("Tokens: {}", fmt::join(output, ","));
}

struct ai_train_binding_weights_post_args {
  std::string text_{};

  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_post_args& out) {
    if (in_json.contains("text") && in_json.at("text").is_string()) in_json.at("text").get_to(out.text_);
  }
};

}  // namespace
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  boost::asio::post(g_io_context(), [l_args]() {
#ifndef NDEBUG
    run_tokenizer(l_args.text_);
#endif
  });
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http