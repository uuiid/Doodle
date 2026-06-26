//
// Created by TD on 25-6-26.
//
#pragma once

#include "llm2vec_tokenizer.h"

#include <doodle_lib/core/global_function.h>

#include <array>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

namespace doodle::http {

/// @brief LLM2Vec 模型推理封装
/// 对应 Python LLM2Vec 类
/// 调用链: operator() → tokenize → ONNX Run → apply_pooling → return embedding
struct LLM2Vec {
  std::unique_ptr<llm2vec_tokenizer> tokenizer_;
  FSys::path model_path_;
  FSys::path tokenizer_json_path_;

  // Pooling 配置（对应 Python pooling_mode / skip_instruction）
  std::string pooling_mode_{"mean"};
  bool skip_instruction_{true};

  // ONNX Runtime session
  std::unique_ptr<Ort::Session> session_;
  std::unique_ptr<Ort::IoBinding> io_binding_;
  std::vector<std::string> input_names_;
  std::vector<std::string> output_names_;
  Ort::MemoryInfo memory_info_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};

  std::once_flag session_init_flag_;

  LLM2Vec() = default;
  explicit LLM2Vec(const FSys::path& in_model_path, const FSys::path& in_tokenizer_json_path);

  /// @brief 执行完整推理管线：tokenize → ONNX Run → pooling
  /// @return embedding 向量（维度 = hidden_size）
  std::vector<float_t> operator()(const std::string& instruction, const std::string& text);

 private:
  /// @brief 延迟初始化 ONNX session（线程安全，仅执行一次）
  void init_session();

  /// @brief 对模型输出做 pooling（对应 Python LLM2Vec.get_pooling）
  std::vector<float_t> apply_pooling(
      const llm2vec_tokenizer::tokenize_result& tokenized, const float* output_data, std::int64_t seq_len,
      std::int64_t hidden_size
  ) const;
};

}  // namespace doodle::http
