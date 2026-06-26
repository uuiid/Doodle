#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/hf_tokenizer.h>
#include <doodle_lib/core/global_function.h>

#include <boost/asio/post.hpp>

#include "ai_main.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <mutex>
#include <numeric>
#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

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

using doodle::hf_tokenizer;

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

  /// @brief 对输入做 instruction 包装（对应 Python prepare_for_tokenization）
  /// LLM2Vec 编码模式使用 指令-文本 配对结构，通过 !@#$%^&*() 分隔符将指令和文本分开
  /// 输出格式: "<|start_header_id|>user<|end_header_id|>\n\n{instruction} !@#$%^&*(){text}<|eot_id|>"
  std::string prepare_for_tokenization(const std::string& instruction, const std::string& text) {
    if (instruction.empty()) {
      return fmt::format("<|start_header_id|>user<|end_header_id|>\n\n{}{}<|eot_id|>", separator_, text);
    }
    return fmt::format("<|start_header_id|>user<|end_header_id|>\n\n{} {}{}<|eot_id|>", instruction, separator_, text);
  }

  /// @brief 两步分词：对应 Python LLM2Vec.tokenize()
  /// 1) 对完整文本（含 instruction）用 add_special_tokens=true 分词
  /// 2) 对纯文本部分用 add_special_tokens=false 分词
  /// 3) 生成 embed_mask（右对齐标记文本部分位置）
  /// 注意：Python padding=True 是动态 padding 到 batch 最长的序列，不是到 max_length
  /// 此处单序列时不做 padding，由上层调用方统一做 batch padding
  tokenize_result tokenize(const std::string& instruction, const std::string& text) {
    // 构建完整文本：<|start_header_id|>user<|end_header_id|>\n\n{instruction} !@#$%^&*(){text}<|eot_id|>
    auto l_text        = prepare_for_tokenization(instruction, text);

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
};

// 对应 python LLM2Vec 模型
// 调用链: operator() → tokenize → ONNX Run → apply_pooling → return embedding
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
  explicit LLM2Vec(const FSys::path& in_model_path, const FSys::path& in_tokenizer_json_path)
      : model_path_(in_model_path), tokenizer_json_path_(in_tokenizer_json_path) {
    tokenizer_ = std::make_unique<llm2vec_tokenizer>(tokenizer_json_path_);
  }

 private:
  /// @brief 延迟初始化 ONNX session（线程安全，仅执行一次）
  void init_session() {
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

  /// @brief 对模型输出做 pooling（对应 Python LLM2Vec.get_pooling）
  /// @param tokenized 分词结果（含 embed_mask）
  /// @param output_data ONNX 输出的 last_hidden_state 数据指针
  /// @param seq_len 序列长度
  /// @param hidden_size 隐藏层维度
  /// @return pooling 后的 embedding 向量
  std::vector<float_t> apply_pooling(
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
      start = seq_len - valid_len;  // 从右侧开始数，跳过 instruction token，只对文本 token 做 pooling
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

 public:
  /// @brief 执行完整推理管线：tokenize → ONNX Run → pooling
  /// @return embedding 向量（维度 = hidden_size）
  std::vector<float_t> operator()(const std::string& instruction, const std::string& text) {
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
    auto memory_info      = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

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
    DOODLE_CHICK(ort_outputs.size() == 1, "Expected exactly one output from ONNX Runtime, got {}", ort_outputs.size());

    // Step 6: 解析输出 shape
    auto type_info    = ort_outputs.front().GetTensorTypeAndShapeInfo();
    auto output_shape = type_info.GetShape();
    if (output_shape.size() != 3) {
      SPDLOG_ERROR("Unexpected ONNX output shape rank: {}", output_shape.size());
      return {};
    }
    const std::int64_t hidden_size = output_shape.back();

    // Step 7: 获取 last_hidden_state 数据并执行 pooling
    float* output_data             = ort_outputs.front().GetTensorMutableData<std::float_t>();

    return apply_pooling(tokenized, output_data, seq_len, hidden_size);
  }
};

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