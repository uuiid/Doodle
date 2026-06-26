//
// Created by TD on 25-6-26.
//
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <tokenizers_c.h>

namespace doodle {

/// @brief HuggingFace tokenizers C++ 包装器
/// 基于 tokenizers 库，提供 encode/decode/vocab 等基础操作
struct hf_tokenizer {
 private:
  std::unique_ptr<void, void (*)(void*)> handle_{nullptr, [](void* ptr) {
                                                   if (ptr) tokenizers_free(ptr);
                                                 }};

 public:
  explicit hf_tokenizer(const std::string& in_tokenizer_json);

  std::vector<int32_t> encode(const std::string& text, bool add_special_tokens = true);

  std::vector<std::vector<int32_t>> EncodeBatch(
      const std::vector<std::string>& texts, bool add_special_tokens = true
  );

  // use i32 to be consistent with sentencepiece
  std::string Decode(const std::vector<int32_t>& ids, bool skip_special_tokens);

  std::string Decode(const std::vector<int32_t>& ids);

  size_t GetVocabSize();

  std::string IdToToken(int32_t id);

  int32_t TokenToId(const std::string& token);
};

}  // namespace doodle
