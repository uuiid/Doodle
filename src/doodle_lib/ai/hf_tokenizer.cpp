//
// Created by TD on 25-6-26.
//
#include "hf_tokenizer.h"

#include <cassert>
#include <stdexcept>

namespace doodle {

hf_tokenizer::hf_tokenizer(const std::string& in_tokenizer_json) {
  auto handle = tokenizers_new_from_str(in_tokenizer_json.data(), in_tokenizer_json.size());
  if (handle == nullptr) throw std::runtime_error("Failed to create tokenizer from JSON");

  handle_.reset(handle);
}

std::vector<int32_t> hf_tokenizer::encode(const std::string& text, bool add_special_tokens) {
  TokenizerEncodeResult result;
  tokenizers_encode(handle_.get(), text.data(), text.length(), static_cast<int>(add_special_tokens), &result);
  std::vector<int32_t> ret(result.token_ids, result.token_ids + result.len);
  tokenizers_free_encode_results(&result, 1);
  return ret;
}

std::vector<std::vector<int32_t>> hf_tokenizer::EncodeBatch(
    const std::vector<std::string>& texts, bool add_special_tokens
) {
  std::vector<const char*> texts_raw;
  std::vector<size_t> seq_lens;
  texts_raw.reserve(texts.size());
  seq_lens.reserve(texts.size());
  for (const auto& text : texts) {
    texts_raw.push_back(text.data());
    seq_lens.push_back(text.length());
  }
  std::vector<TokenizerEncodeResult> results(texts.size());
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

std::string hf_tokenizer::Decode(const std::vector<int32_t>& ids, bool skip_special_tokens) {
  tokenizers_decode(
      handle_.get(), reinterpret_cast<const uint32_t*>(ids.data()), ids.size(), static_cast<int>(skip_special_tokens)
  );
  const char* data;
  size_t len;
  tokenizers_get_decode_str(handle_.get(), &data, &len);
  return std::string(data, len);
}

std::string hf_tokenizer::Decode(const std::vector<int32_t>& ids) { return Decode(ids, false); }

size_t hf_tokenizer::GetVocabSize() {
  size_t size;
  tokenizers_get_vocab_size(handle_.get(), &size);
  assert(size > 0);
  return size;
}

std::string hf_tokenizer::IdToToken(int32_t id) {
  const char* data;
  size_t len;
  tokenizers_id_to_token(handle_.get(), static_cast<uint32_t>(id), &data, &len);
  return std::string(data, len);
}

int32_t hf_tokenizer::TokenToId(const std::string& token) {
  int32_t id;
  tokenizers_token_to_id(handle_.get(), token.data(), token.length(), &id);
  return id;
}

}  // namespace doodle
