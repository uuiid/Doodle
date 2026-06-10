#include <doodle_lib/core/global_function.h>

#include <boost/asio/post.hpp>

#include "ai_main.h"
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <string>
#include <tokenizers_c.h>
#include <tokenizers_cpp.h>
#include <vector>

namespace doodle::http {
namespace {
// 我们自己包装的分词器，基于 tokenizers 库，适配 LLM2Vec 的编码模式
struct hf_tokenizer {
 private:
  std::unique_ptr<void, void (*)(void*)> handle_{nullptr, [](void* ptr) {
                                                   if (ptr) tokenizers_free(ptr);
                                                 }};

 public:
  explicit hf_tokenizer(const std::string& in_tokenizer_json) {
    auto handle = tokenizers_new_from_str(in_tokenizer_json.data(), in_tokenizer_json.size());
    if (handle == nullptr) throw std::runtime_error("Failed to create tokenizer from JSON");

    handle_.reset(handle);
  }

  std::vector<int32_t> encode(const std::string& text, bool add_special_tokens = true) {
    TokenizerEncodeResult result;
    tokenizers_encode(handle_.get(), text.data(), text.length(), static_cast<int>(add_special_tokens), &result);
    std::vector<int32_t> ret(result.token_ids, result.token_ids + result.len);
    tokenizers_free_encode_results(&result, 1);
    return ret;
  }
  std::vector<std::vector<int32_t>> EncodeBatch(const std::vector<std::string>& texts, bool add_special_tokens = true) {
    std::vector<const char*> texts_raw;
    std::vector<size_t> seq_lens;
    size_t num_seqs = texts.size();
    texts_raw.reserve(num_seqs);
    seq_lens.reserve(num_seqs);
    for (const auto& text : texts) {
      texts_raw.push_back(text.data());
      seq_lens.push_back(text.length());
    }
    std::vector<TokenizerEncodeResult> results(num_seqs);
    tokenizers_encode_batch(
        handle_.get(), texts_raw.data(), seq_lens.data(), texts.size(), static_cast<int>(add_special_tokens),
        results.data()
    );
    std::vector<std::vector<int32_t>> ret;
    ret.reserve(texts.size());
    for (size_t i = 0; i < texts.size(); ++i) {
      ret.push_back(std::vector<int32_t>(results[i].token_ids, results[i].token_ids + results[i].len));
    }
    tokenizers_free_encode_results(results.data(), texts.size());
    return ret;
  }

  // use i32 to be consistent with sentencepiece
  std::string Decode(const std::vector<int32_t>& ids, bool skip_special_tokens) {
    tokenizers_decode(
        handle_.get(), reinterpret_cast<const uint32_t*>(ids.data()), ids.size(), static_cast<int>(skip_special_tokens)
    );
    const char* data;
    size_t len;
    tokenizers_get_decode_str(handle_.get(), &data, &len);
    return std::string(data, len);
  }

  std::string Decode(const std::vector<int32_t>& ids) { return Decode(ids, false); }

  size_t GetVocabSize() {
    size_t size;
    tokenizers_get_vocab_size(handle_.get(), &size);
    assert(size > 0);
    return size;
  }

  std::string IdToToken(int32_t id) {
    const char* data;
    size_t len;
    tokenizers_id_to_token(handle_.get(), static_cast<uint32_t>(id), &data, &len);
    return std::string(data, len);
  }

  int32_t TokenToId(const std::string& token) {
    int32_t id;
    tokenizers_token_to_id(handle_.get(), token.data(), token.length(), &id);
    return id;
  }
};

struct llm2vec_tokenizer {
  std::unique_ptr<hf_tokenizer> tokenizer_;
  std::size_t max_length_{512};
  std::size_t doc_max_length_{400};
  std::string separator_{"!@#$%^&*()"};

  // 分词输出
  struct tokenize_result {
    std::vector<int64_t> input_ids;       // 完整文本的 token IDs（已 padding）
    std::vector<int64_t> attention_mask;  // attention mask
    std::vector<int64_t> embed_mask;      // 文本部分的 mask（1=文本, 0=instruction）
    std::size_t seq_len;                  // 实际序列长度（未 padding 前）
  };
  // 创建分词器
  auto create_tokenizer(const FSys::path& in_tokenizer_json_path) {
    FSys::ifstream l_file(in_tokenizer_json_path);
    auto l_json_str = std::string{std::istreambuf_iterator<char>(l_file), std::istreambuf_iterator<char>()};
    return std::make_unique<hf_tokenizer>(l_json_str);
  }

  explicit llm2vec_tokenizer(const FSys::path& in_tokenizer_json_path) {
    tokenizer_ = create_tokenizer(in_tokenizer_json_path);
  }

  // 分两步：先包装 instruction，再分流分割处理
  tokenize_result tokenize(const std::string& instruction, const std::string& text) {
    auto l_text = prepare_for_tokenization(instruction, text);
    std::string l_text2{};
    std::string l_original_texts{};

    {
      std::vector<std::string> l_texts{};
      boost::algorithm::split(l_texts, l_text, boost::algorithm::is_any_of(separator_));
      l_text2          = l_texts.size() > 1 ? l_texts[1] : std::string{};
      l_original_texts = fmt::to_string(fmt::join(l_texts, ""));
    }
  }

  // 对输入做 instruction 包装（对应 prepare_for_tokenization）
  // LLM2Vec 的编码模式使用 指令-文本 配对 结构, 通过 !@#$%^&*() 分隔符将指令和文本分开, 以确保分词器正确识别文本部分
  // meta-llama/Meta-Llama-3-8B-Instruct 分词器需要继续添加关键字 "<|start_header_id|>user<|end_header_id|>\n\n" +
  // text.strip() + "<|eot_id|>"
  std::string prepare_for_tokenization(const std::string& instruction, const std::string& text) {
    return fmt::format("<|start_header_id|>user<|end_header_id|>\n\n{}{}{}<|eot_id|>", instruction, text, separator_);
  }
};

// 运行分词器
void run_tokenizer(const FSys::path& in_tokenizer_json_path, const std::string& in_text) {
  if (in_text.empty()) return SPDLOG_INFO("Input text is empty, skipping tokenization.");

  llm2vec_tokenizer tokenizer{in_tokenizer_json_path};
  auto result = tokenizer.tokenize("", in_text);
  SPDLOG_INFO("Tokens: {}", fmt::join(result.input_ids, ","));
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
    run_tokenizer("D:\\ai_mod\\onnx-McGill-NLP--LLM2Vec-Meta-Llama-3-8B-Instruct-mntp\\tokenizer.json", l_args.text_);
#endif
  });
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http