//
// Created by TD on 25-6-26.
//
#include "llm2vec_tokenizer.h"

#include <algorithm>
#include <fmt/format.h>

namespace doodle::http {

llm2vec_tokenizer::llm2vec_tokenizer(const FSys::path& in_tokenizer_json_path) {
  FSys::ifstream l_file(in_tokenizer_json_path);
  auto l_json_str = std::string{std::istreambuf_iterator<char>(l_file), std::istreambuf_iterator<char>()};
  tokenizer_      = std::make_unique<doodle::hf_tokenizer>(l_json_str);
}

std::string llm2vec_tokenizer::prepare_for_tokenization(
    const std::string& instruction, const std::string& text
) {
  if (instruction.empty()) {
    return fmt::format("<|start_header_id|>user<|end_header_id|>\n\n{}{}<|eot_id|>", separator_, text);
  }
  return fmt::format(
      "<|start_header_id|>user<|end_header_id|>\n\n{} {}{}<|eot_id|>", instruction, separator_, text
  );
}

llm2vec_tokenizer::tokenize_result llm2vec_tokenizer::tokenize(
    const std::string& instruction, const std::string& text
) {
  // 构建完整文本：<|start_header_id|>user<|end_header_id|>\n\n{instruction} !@#$%^&*(){text}<|eot_id|>
  auto l_text = prepare_for_tokenization(instruction, text);

  // Python 用 str.split("!@#$%^&*()") 以完整字符串为分隔符拆分
  // 不能用 boost::is_any_of（那是按单个字符拆分）
  const auto sep_pos = l_text.find(separator_);
  std::string l_text2{};
  std::string l_original_texts{};
  if (sep_pos != std::string::npos) {
    l_text2 = l_text.substr(sep_pos + separator_.size());  // 分隔符之后 → 文本部分
    l_original_texts =
        l_text.substr(0, sep_pos) + l_text.substr(sep_pos + separator_.size());  // 去掉分隔符的完整文本
  } else {
    l_original_texts = l_text;
  }

  // === 第一次分词：完整文本，带特殊 token（对应 Python: tokenizer(original_texts, add_special_tokens=True)）===
  auto full_ids = tokenizer_->encode(l_original_texts, true);

  // === 第二次分词：仅文本部分，不加特殊 token（对应 Python: tokenizer([t], add_special_tokens=False)）===
  auto text_ids = tokenizer_->encode(l_text2, false);

  // 从右侧截断到 max_length
  if (full_ids.size() > max_length_) full_ids.resize(max_length_);
  if (text_ids.size() > max_length_) text_ids.resize(max_length_);

  // Python 对单序列不做 padding，只输出实际长度
  tokenize_result result{};
  result.seq_len = full_ids.size();
  result.input_ids.assign(full_ids.begin(), full_ids.end());
  result.attention_mask.resize(full_ids.size(), 1);  // 无 padding，全是 1
  result.embed_mask.resize(full_ids.size(), 0);

  // 生成 embed_mask：文本 token 右对齐标记
  // Python 对应: e_m[-len(ids["input_ids"][0]):] = 1
  const size_t text_len = (std::min)(text_ids.size(), full_ids.size());
  if (text_len > 0) {
    const size_t text_start = full_ids.size() - text_len;
    for (size_t i = text_start; i < full_ids.size(); ++i) {
      result.embed_mask[i] = 1;
    }
  }

  return result;
}

}  // namespace doodle::http
