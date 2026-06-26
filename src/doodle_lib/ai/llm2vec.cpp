//
// Created by TD on 25-6-26.
//
#include "llm2vec.h"

#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <numeric>
#include <spdlog/spdlog.h>

#include "doodle_core/exception/exception.h"

namespace doodle::http {

LLM2Vec::LLM2Vec(const FSys::path& in_model_path, const FSys::path& in_tokenizer_json_path)
    : model_path_(in_model_path), tokenizer_json_path_(in_tokenizer_json_path) {
  tokenizer_ = std::make_unique<llm2vec_tokenizer>(tokenizer_json_path_);
}

void LLM2Vec::init_session() {
  Ort::SessionOptions session_options;
  // session_options.SetIntraOpNumThreads(1);
  session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
  session_      = std::make_unique<Ort::Session>(get_ort_env(), model_path_.wstring().c_str(), session_options);
  input_names_  = session_->GetInputNames();
  output_names_ = session_->GetOutputNames();
  io_binding_   = std::make_unique<Ort::IoBinding>(*session_);
  // 绑定输出到 CPU memory，让 ONNX Runtime 自动分配输出 tensor，避免传空 OrtValue* 导致空指针崩溃
  for (const auto& name : output_names_) io_binding_->BindOutput(name.c_str(), memory_info_);

  SPDLOG_INFO(
      "ONNX Runtime session 初始化成功，输入: [{}], 输出: [{}]", fmt::join(input_names_, ","),
      fmt::join(output_names_, ",")
  );
}

std::vector<float_t> LLM2Vec::apply_pooling(
    const llm2vec_tokenizer::tokenize_result& tokenized, const float* output_data, std::int64_t seq_len,
    std::int64_t hidden_size
) const {
  std::vector<float_t> result(hidden_size, 0.0f);

  // 确定实际参与 pooling 的 token 范围（对应 Python _skip_instruction）
  std::int64_t valid_len = seq_len;
  std::int64_t start     = 0;
  if (skip_instruction_) {
    // embed_mask == 1 表示文本 token（右对齐），0 表示 instruction token
    // 相加得到文本 token 的数量，从右侧开始数，跳过 instruction token，只对文本 token 做 pooling
    valid_len = std::reduce(tokenized.embed_mask.begin(), tokenized.embed_mask.end());
    start     = seq_len - valid_len;  // 从右侧开始数，跳过 instruction token，只对文本 token 做 pooling
  }

  if (valid_len <= 0) {
    SPDLOG_WARN("No valid tokens for pooling after skip_instruction");
    return result;
  }

  // ---- pooling_modes（对应 Python get_pooling 各分支） ----
  if (pooling_mode_ == "mean") {
    // mean pooling：对所有有效 token 的 hidden_state 取平均
    for (std::int64_t i = start; i < seq_len; ++i) {
      for (std::int64_t j = 0; j < hidden_size; ++j) {
        result[j] += output_data[i * hidden_size + j];
      }
    }
    for (auto& v : result) v /= static_cast<float_t>(valid_len);

  } else if (pooling_mode_ == "weighted_mean") {
    // weighted mean pooling：位置越靠后权重越大
    for (std::int64_t i = start; i < seq_len; ++i) {
      const float_t weight =
          static_cast<float_t>(i - start + 1) / static_cast<float_t>(valid_len * (valid_len + 1) / 2);
      for (std::int64_t j = 0; j < hidden_size; ++j) {
        result[j] += output_data[i * hidden_size + j] * weight;
      }
    }

  } else if (pooling_mode_ == "eos_token" || pooling_mode_ == "last_token") {
    // 取最后一个 token 的 hidden_state
    for (std::int64_t j = 0; j < hidden_size; ++j) {
      result[j] = output_data[(seq_len - 1) * hidden_size + j];
    }

  } else {
    SPDLOG_WARN("Unsupported pooling_mode '{}', falling back to mean", pooling_mode_);
    for (std::int64_t i = start; i < seq_len; ++i) {
      for (std::int64_t j = 0; j < hidden_size; ++j) {
        result[j] += output_data[i * hidden_size + j];
      }
    }
    for (auto& v : result) v /= static_cast<float_t>(valid_len);
  }

  return result;
}

std::vector<float_t> LLM2Vec::operator()(const std::string& instruction, const std::string& text) {
  // Step 1: 分词（由 llm2vec_tokenizer 完成 instruction 包装 + 两步分词 + embed_mask）
  auto tokenized = tokenizer_->tokenize(instruction, text);
  if (tokenized.input_ids.empty()) {
    SPDLOG_WARN("Tokenization returned empty input_ids");
    return {};
  }

  // Step 2: 延迟初始化 ONNX session
  std::call_once(session_init_flag_, &LLM2Vec::init_session, this);

  // Step 3: 创建 ONNX Runtime 输入 tensor
  const std::int64_t seq_len = static_cast<std::int64_t>(tokenized.input_ids.size());
  const std::array<std::int64_t, 2> input_shape{1, seq_len};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  auto input_ids_tensor = Ort::Value::CreateTensor<std::int64_t>(
      memory_info, tokenized.input_ids.data(), tokenized.input_ids.size(), input_shape.data(), input_shape.size()
  );
  auto attn_mask_tensor = Ort::Value::CreateTensor<std::int64_t>(
      memory_info, tokenized.attention_mask.data(), tokenized.attention_mask.size(), input_shape.data(),
      input_shape.size()
  );

  io_binding_->BindInput("input_ids", input_ids_tensor);
  io_binding_->BindInput("attention_mask", attn_mask_tensor);

  // 部分 ONNX 导出包含 position_ids 输入（Llama 等模型的 rotary embedding 需要）
  // 生成 position_ids: [0, 1, 2, ..., seq_len-1] shape {1, seq_len}
  // 注意：position_ids 必须在此作用域保持存活直至 Run 完成
  std::vector<std::int64_t> position_ids;
  if (std::find_if(input_names_.begin(), input_names_.end(), [](const std::string& name) {
        return name == "position_ids" || name == "position_ids_1";
      }) != input_names_.end()) {
    position_ids.assign(static_cast<std::size_t>(seq_len), 0);
    io_binding_->BindInput(
        "position_ids",
        Ort::Value::CreateTensor<std::int64_t>(
            memory_info, position_ids.data(), position_ids.size(), input_shape.data(), input_shape.size()
        )
    );
  }
  // Step 4: 运行模型推理
  try {
    session_->Run(Ort::RunOptions{nullptr}, *io_binding_);
  } catch (const Ort::Exception& e) {
    return SPDLOG_ERROR("ONNX Runtime inference failed: {}", e.what()), std::vector<float_t>{};
  }

  // Step 5: 获取输出（ORT 自动分配内存）
  auto ort_outputs = io_binding_->GetOutputValues();
  if (ort_outputs.empty()) return SPDLOG_ERROR("ONNX Runtime returned no outputs"), std::vector<float_t>{};
  DOODLE_CHICK(
      ort_outputs.size() == 1, "Expected exactly one output from ONNX Runtime, got {}", ort_outputs.size()
  );

  // Step 6: 解析输出 shape
  auto type_info    = ort_outputs.front().GetTensorTypeAndShapeInfo();
  auto output_shape = type_info.GetShape();
  if (output_shape.size() != 3) {
    SPDLOG_ERROR("Unexpected ONNX output shape rank: {}", output_shape.size());
    return {};
  }
  const std::int64_t hidden_size = output_shape.back();

  // Step 7: 获取 last_hidden_state 数据并执行 pooling
  float* output_data = ort_outputs.front().GetTensorMutableData<std::float_t>();

  return apply_pooling(tokenized, output_data, seq_len, hidden_size);
}

}  // namespace doodle::http
