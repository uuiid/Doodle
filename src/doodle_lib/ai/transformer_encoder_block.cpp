//
// Created by TD on 25-6-29.
//
#include "transformer_encoder_block.h"

#include <doodle_core/exception/exception.h>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <array>

namespace doodle::ai {

void TransformerEncoderBlock::load(
    const FSys::path& model_dir,
    std::int64_t latent_dim,
    std::int64_t num_text_tokens,
    bool use_text_mask,
    bool input_first_heading_angle
) {
  model_dir_                = model_dir;
  latent_dim_                = latent_dim;
  num_text_tokens_           = num_text_tokens;
  use_text_mask_             = use_text_mask;
  input_first_heading_angle_ = input_first_heading_angle;

  // 加载 Linear 层
  embed_text_.load(model_dir / "embed_text_weight.npy", model_dir / "embed_text_bias.npy");
  input_linear_.load(model_dir / "input_linear_weight.npy", model_dir / "input_linear_bias.npy");
  output_linear_.load(model_dir / "output_linear_weight.npy", model_dir / "output_linear_bias.npy");

  DOODLE_CHICK(
      embed_text_.out_features() == latent_dim_,
      "embed_text out_features {} 不匹配 latent_dim {}", embed_text_.out_features(), latent_dim_
  );
  DOODLE_CHICK(
      input_linear_.out_features() == latent_dim_,
      "input_linear out_features {} 不匹配 latent_dim {}", input_linear_.out_features(), latent_dim_
  );
  DOODLE_CHICK(
      output_linear_.in_features() == latent_dim_,
      "output_linear in_features {} 不匹配 latent_dim {}", output_linear_.in_features(), latent_dim_
  );

  // 可选：初始朝向角线性层
  if (input_first_heading_angle_) {
    linear_first_heading_angle_.load(
        model_dir / "linear_first_heading_angle_weight.npy",
        model_dir / "linear_first_heading_angle_bias.npy"
    );
    DOODLE_CHICK(
        linear_first_heading_angle_.out_features() == latent_dim_,
        "linear_first_heading_angle out_features {} 不匹配 latent_dim {}",
        linear_first_heading_angle_.out_features(), latent_dim_
    );
  }

  // 初始化位置编码（max_len=5000 匹配 Python 默认值）
  sequence_pos_encoder_.init(latent_dim_, 5000);

  // 初始化时间步编码器
  embed_timestep_.init(
      latent_dim_,
      &sequence_pos_encoder_,
      model_dir / "timestep_linear1_weight.npy",
      model_dir / "timestep_linear1_bias.npy",
      model_dir / "timestep_linear2_weight.npy",
      model_dir / "timestep_linear2_bias.npy"
  );

  SPDLOG_INFO(
      "TransformerEncoderBlock 加载完成: latent_dim={}, num_text_tokens={}, "
      "use_text_mask={}, input_first_heading_angle={}",
      latent_dim_, num_text_tokens_, use_text_mask_, input_first_heading_angle_
  );
}

void TransformerEncoderBlock::init_session() {
  Ort::SessionOptions session_options;
  session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  auto onnx_path = model_dir_ / "seq_trans_encoder.onnx";
  DOODLE_CHICK(FSys::exists(onnx_path), "ONNX 模型文件不存在: {}", onnx_path.string());

  session_ = std::make_unique<Ort::Session>(get_ort_env(), onnx_path.wstring().c_str(), session_options);
  input_names_  = session_->GetInputNames();
  output_names_ = session_->GetOutputNames();
  io_binding_   = std::make_unique<Ort::IoBinding>(*session_);

  for (const auto& name : output_names_) {
    io_binding_->BindOutput(name.c_str(), memory_info_);
  }

  SPDLOG_INFO(
      "seqTransEncoder ONNX session 初始化成功，输入: [{}], 输出: [{}]",
      fmt::join(input_names_, ","), fmt::join(output_names_, ",")
  );
}

Eigen::MatrixXf TransformerEncoderBlock::forward(
    const Eigen::MatrixXf& x,
    const MatrixXb& x_pad_mask,
    const Eigen::MatrixXf& text_feat,
    const MatrixXb& text_feat_pad_mask,
    const std::vector<std::int64_t>& timesteps,
    const std::vector<float>& first_heading_angle
) {
  // ---- 提取形状信息 ----
  // x: [B*T, input_dim]
  // x_pad_mask: [B, T]
  // text_feat: [B*max_text_len, llm_dim]
  // text_feat_pad_mask: [B, max_text_len]  (在 collate 中可能已 pad 到 num_text_tokens)
  // timesteps: [B]
  const auto batch_size      = static_cast<Eigen::Index>(x_pad_mask.rows());
  const auto time_steps      = static_cast<Eigen::Index>(x_pad_mask.cols());
  const auto max_text_len    = static_cast<Eigen::Index>(text_feat_pad_mask.cols());

  DOODLE_CHICK(
      x.rows() == batch_size * time_steps,
      "x rows {} 不匹配 batch_size*time_steps {}*{}", x.rows(), batch_size, time_steps
  );
  DOODLE_CHICK(
      static_cast<Eigen::Index>(timesteps.size()) == batch_size,
      "timesteps size {} 不匹配 batch_size {}", timesteps.size(), batch_size
  );

  // ---- Step 1: x = input_linear(x)  [B*T, input_dim] -> [B*T, latent_dim] ----
  Eigen::MatrixXf x_emb = input_linear_.forward(x);  // [B*T, latent_dim]

  // ---- Step 2: Pad text tokens 到固定大小 num_text_tokens ----
  // text_feat: [B*max_text_len, llm_dim]
  // 需要 reshape 为 [B, max_text_len, llm_dim] 然后 pad
  // 我们用平坦化方式处理
  Eigen::MatrixXf text_padded;  // [B*num_text_tokens, llm_dim]
  MatrixXb text_mask_padded;  // [B, num_text_tokens]

  {
    const auto llm_dim = text_feat.cols();
    if (max_text_len == num_text_tokens_) {
      // 已经 pad 好了
      text_padded       = text_feat;
      text_mask_padded  = text_feat_pad_mask;
    } else {
      // 需要 pad
      text_padded.resize(batch_size * num_text_tokens_, llm_dim);
      text_padded.setZero();
      text_mask_padded.resize(batch_size, num_text_tokens_);
      text_mask_padded.setZero();

      const auto copy_len = (std::min)(max_text_len, static_cast<Eigen::Index>(num_text_tokens_));
      for (Eigen::Index b = 0; b < batch_size; ++b) {
        // 复制文本特征
        text_padded.block(b * num_text_tokens_, 0, copy_len, llm_dim) =
            text_feat.block(b * max_text_len, 0, copy_len, llm_dim);
        // 复制 mask
        text_mask_padded.block(b, 0, 1, copy_len) =
            text_feat_pad_mask.block(b, 0, 1, copy_len);
      }

      if (max_text_len > num_text_tokens_) {
        SPDLOG_WARN("文本 token 数 {} 超过最大 {}, 裁剪", max_text_len, num_text_tokens_);
      }
    }
  }

  // ---- Step 3: emb_text = embed_text(text_padded)  [B*num_text_tokens, latent_dim] ----
  Eigen::MatrixXf emb_text = embed_text_.forward(text_padded);  // [B*num_text_tokens, latent_dim]

  // ---- Step 4: emb_time = embed_timestep(timesteps)  [B, latent_dim] (含 [B,1,D] 语义) ----
  Eigen::MatrixXf emb_time = embed_timestep_.forward(timesteps);  // [B, latent_dim]

  // ---- Step 5: 构建 prefix 特征和 mask ----
  // prefix_feats = [emb_text | emb_time]  -> [B, num_text_tokens + 1, latent_dim]
  // 平坦化: [B*(num_text_tokens+1), latent_dim]

  Eigen::Index prefix_len = num_text_tokens_ + 1;

  // 处理 text_feat_pad_mask: 如果 use_text_mask = false, 则全 true
  MatrixXb text_mask_used = text_mask_padded;
  if (!use_text_mask_) {
    text_mask_used.resize(batch_size, num_text_tokens_);
    text_mask_used.setConstant(true);
  }

  // time_mask: [B, 1], 全 true
  MatrixXb time_mask(batch_size, 1);
  time_mask.setConstant(true);

  // ---- 可选: first_heading_angle ----
  Eigen::MatrixXf first_heading_angle_feats;
  MatrixXb first_heading_angle_mask;
  if (input_first_heading_angle_ && !first_heading_angle.empty()) {
    DOODLE_CHICK(
        static_cast<Eigen::Index>(first_heading_angle.size()) == batch_size,
        "first_heading_angle size {} 不匹配 batch_size {}", first_heading_angle.size(), batch_size
    );

    // cos(angle), sin(angle) -> [B, 2]
    first_heading_angle_feats.resize(batch_size, 2);
    for (Eigen::Index b = 0; b < batch_size; ++b) {
      first_heading_angle_feats(b, 0) = std::cos(first_heading_angle[static_cast<std::size_t>(b)]);
      first_heading_angle_feats(b, 1) = std::sin(first_heading_angle[static_cast<std::size_t>(b)]);
    }

    // linear_first_heading_angle: [B, 2] -> [B, latent_dim]
    first_heading_angle_feats = linear_first_heading_angle_.forward(first_heading_angle_feats);

    first_heading_angle_mask.resize(batch_size, 1);
    first_heading_angle_mask.setConstant(true);

    prefix_len += 1;
  }

  // ---- 拼接 prefix：text + time + (可选) angle  ----
  // prefix_feats: [B, prefix_len, latent_dim] 平坦化为 [B*prefix_len, latent_dim]
  Eigen::MatrixXf prefix_feats(batch_size * prefix_len, latent_dim_);

  // 填充 text 部分
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    prefix_feats.block(b * prefix_len, 0, num_text_tokens_, latent_dim_) =
        emb_text.block(b * num_text_tokens_, 0, num_text_tokens_, latent_dim_);
  }
  // 填充 time 部分
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    prefix_feats.row(b * prefix_len + num_text_tokens_) = emb_time.row(b);
  }
  // 填充 angle 部分
  if (input_first_heading_angle_ && !first_heading_angle.empty()) {
    for (Eigen::Index b = 0; b < batch_size; ++b) {
      prefix_feats.row(b * prefix_len + num_text_tokens_ + 1) = first_heading_angle_feats.row(b);
    }
  }

  // ---- 拼接 mask：text_mask + time_mask + (可选) angle_mask ----
  // prefix_mask: [B, prefix_len]
  MatrixXb prefix_mask(batch_size, prefix_len);
  prefix_mask.block(0, 0, batch_size, num_text_tokens_) = text_mask_used;
  prefix_mask.col(num_text_tokens_)                      = time_mask.col(0);
  if (input_first_heading_angle_ && !first_heading_angle.empty()) {
    prefix_mask.col(num_text_tokens_ + 1) = first_heading_angle_mask.col(0);
  }

  // ---- 计算 pose_start_ind = prefix_len ----
  const auto pose_start_ind = prefix_len;

  // ---- 拼接 prefix 和 x: xseq [B, prefix_len+T, latent_dim] ----
  const auto total_len = prefix_len + time_steps;
  Eigen::MatrixXf xseq(batch_size * total_len, latent_dim_);

  // 填充 prefix
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    xseq.block(b * total_len, 0, prefix_len, latent_dim_) =
        prefix_feats.block(b * prefix_len, 0, prefix_len, latent_dim_);
  }
  // 填充 x
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    xseq.block(b * total_len + prefix_len, 0, time_steps, latent_dim_) =
        x_emb.block(b * time_steps, 0, time_steps, latent_dim_);
  }

  // ---- 拼接 mask 并取反 (ONNX 需要 True=pad) ----
  // Python: src_key_padding_mask = ~torch.cat((prefix_mask, x_pad_mask), axis=1)
  // prefix_mask: True=有效, x_pad_mask: True=有效
  // ~ 取反后: True=pad（需要被 mask 的填充位置）
  MatrixXb src_key_padding_mask(batch_size, total_len);
  src_key_padding_mask.block(0, 0, batch_size, prefix_len)              = prefix_mask;
  src_key_padding_mask.block(0, prefix_len, batch_size, time_steps)     = x_pad_mask;
  src_key_padding_mask = !src_key_padding_mask.array();

  // ---- 添加位置编码 ----
  // xseq: [B*total_len, latent_dim] 需要 reshape 为 [total_len, latent_dim] 逐条加 PE
  // PositionalEncoding::forward 接受 [seq_len, d_model]
  // 我们对每个 batch 分别处理
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    Eigen::MatrixXf seq_slice = xseq.block(b * total_len, 0, total_len, latent_dim_);
    seq_slice = sequence_pos_encoder_.forward(seq_slice);
    xseq.block(b * total_len, 0, total_len, latent_dim_) = seq_slice;
  }

  // ---- 延迟初始化 ONNX session ----
  std::call_once(session_init_flag_, &TransformerEncoderBlock::init_session, this);

  // ---- 准备 ONNX 输入 ----
  DOODLE_CHICK(!input_names_.empty(), "ONNX session 未正确初始化输入名称");

  // ONNX 期望 RowMajor 连续内存，将 Eigen 列主序数据拷贝到行主序 vector
  // xseq: [B*total_len, latent_dim] -> ONNX: [B, total_len, latent_dim]
  std::vector<float> xseq_onnx(static_cast<std::size_t>(batch_size * total_len * latent_dim_));
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < total_len; ++t) {
      for (Eigen::Index d = 0; d < latent_dim_; ++d) {
        xseq_onnx[static_cast<std::size_t>(b * total_len * latent_dim_ + t * latent_dim_ + d)] =
            xseq(b * total_len + t, d);
      }
    }
  }

  // mask: [B, total_len], bool
  std::vector<bool> mask_onnx(static_cast<std::size_t>(batch_size * total_len));
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < total_len; ++t) {
      mask_onnx[static_cast<std::size_t>(b * total_len + t)] = src_key_padding_mask(b, t);
    }
  }

  // 创建 ONNX tensor
  // 确定输入名称
  const std::string& input_data_name  = input_names_[0];
  const std::string& input_mask_name  = (input_names_.size() > 1) ? input_names_[1] : std::string{};

  // 序列输入 shape: [B, total_len, latent_dim]
  std::array<std::int64_t, 3> data_shape{batch_size, total_len, latent_dim_};
  std::array<std::int64_t, 2> mask_shape{batch_size, total_len};

  auto data_tensor = Ort::Value::CreateTensor<float>(
      memory_info_, xseq_onnx.data(), xseq_onnx.size(), data_shape.data(), data_shape.size()
  );

  io_binding_->BindInput(input_data_name.c_str(), data_tensor);

  if (!input_mask_name.empty()) {
    // ONNX 的 mask 通常是 int64 或 bool 类型
    // 用 std::vector<int64_t> 更通用
    std::vector<std::int64_t> mask_int64(static_cast<std::size_t>(batch_size * total_len));
    for (std::size_t i = 0; i < mask_onnx.size(); ++i) {
      mask_int64[i] = mask_onnx[i] ? 1 : 0;
    }

    auto mask_tensor = Ort::Value::CreateTensor<std::int64_t>(
        memory_info_, mask_int64.data(), mask_int64.size(), mask_shape.data(), mask_shape.size()
    );
    io_binding_->BindInput(input_mask_name.c_str(), mask_tensor);
  }

  // ---- 运行 ONNX 推理 ----
  try {
    session_->Run(Ort::RunOptions{nullptr}, *io_binding_);
  } catch (const Ort::Exception& e) {
    DOODLE_CHICK(false, "seqTransEncoder ONNX 推理失败: {}", e.what());
  }

  // ---- 获取 ONNX 输出 ----
  auto ort_outputs = io_binding_->GetOutputValues();
  DOODLE_CHICK(!ort_outputs.empty(), "seqTransEncoder ONNX 无输出");

  auto type_info    = ort_outputs.front().GetTensorTypeAndShapeInfo();
  auto output_shape = type_info.GetShape();
  DOODLE_CHICK(output_shape.size() == 3, "ONNX 输出 shape rank 应为 3, 实际 {}", output_shape.size());

  const std::int64_t out_batch    = output_shape[0];
  const std::int64_t out_seq_len  = output_shape[1];
  const std::int64_t out_dim      = output_shape[2];
  DOODLE_CHICK(
      out_batch == batch_size && out_seq_len == total_len && out_dim == latent_dim_,
      "ONNX 输出 shape [{},{},{}] 不匹配期望 [{},{},{}]",
      out_batch, out_seq_len, out_dim, batch_size, total_len, latent_dim_
  );

  float* onnx_output_data = ort_outputs.front().GetTensorMutableData<float>();

  // ---- 提取 pose_start_ind 之后的运动部分: [B*T, latent_dim] ----
  Eigen::MatrixXf transformer_out(batch_size * time_steps, latent_dim_);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      for (Eigen::Index d = 0; d < latent_dim_; ++d) {
        const auto onnx_idx =
            static_cast<std::size_t>(b * total_len * latent_dim_ + (pose_start_ind + t) * latent_dim_ + d);
        transformer_out(b * time_steps + t, d) = onnx_output_data[onnx_idx];
      }
    }
  }

  // ---- output_linear: [B*T, latent_dim] -> [B*T, output_dim] ----
  Eigen::MatrixXf result = output_linear_.forward(transformer_out);

  return result;
}

}  // namespace doodle::ai
