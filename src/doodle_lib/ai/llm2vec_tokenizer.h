//
// Created by TD on 25-6-26.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_lib/ai/hf_tokenizer.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace doodle::http {

/// @brief LLM2Vec 专用分词器
/// 对应 Python LLM2Vec.tokenize() 的两步分词逻辑
/// 使用 指令-文本 配对结构，通过 !@#$%^&*() 分隔符将指令和文本分开
struct llm2vec_tokenizer {
  std::unique_ptr<doodle::hf_tokenizer> tokenizer_;
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

  explicit llm2vec_tokenizer(const FSys::path& in_tokenizer_json_path);

  /// @brief 对输入做 instruction 包装（对应 Python prepare_for_tokenization）
  std::string prepare_for_tokenization(const std::string& instruction, const std::string& text);

  /// @brief 两步分词：对应 Python LLM2Vec.tokenize()
  tokenize_result tokenize(const std::string& instruction, const std::string& text);
};

}  // namespace doodle::http
