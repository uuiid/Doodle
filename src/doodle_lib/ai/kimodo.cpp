//
// Created by TD on 25-7-23.
//
#include "kimodo.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/motion_rep/feature_utils.h>
#include <doodle_lib/ai/motion_rep/motion_postprocess.h>

#include <Eigen/Core>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <random>
#include <spdlog/spdlog.h>

namespace doodle::ai {

void from_json(const nlohmann::json& j, generate_arg& p) {
  if (j.contains("segments") && j.at("segments").is_array())
    j.at("segments").get_to(p.segments_);
  else if (j.contains("segment") && j.at("segment").is_object()) {
    generate_segment_args single_segment;
    j.at("segment").get_to(single_segment);
    p.segments_.push_back(single_segment);
  }
  if (j.contains("first_heading_angle") && j.at("first_heading_angle").is_number())
    j.at("first_heading_angle").get_to(p.first_heading_angle_);
  if (j.contains("skeleton") && j.at("skeleton").is_object()) {
    p.skeleton_ = std::make_shared<skeleton_base>();
    j.at("skeleton").get_to(*p.skeleton_);
  }
  if (j.contains("root_trajectory") && j.at("root_trajectory").is_array() && j.at("root_trajectory").size() == 3) {
    std::array<std::float_t, 3> root_traj{};
    j.at("root_trajectory").get_to(root_traj);
    p.root_trajectory_ = Eigen::RowVector3f(root_traj[0], root_traj[1], root_traj[2]);
  }
}

void kimodo_model_config::load_from_json(const FSys::path& json_path) {
  DOODLE_CHICK(FSys::exists(json_path), "kimodo_model_config json 文件不存在: {}", json_path.string());
  auto l_json_path = json_path;
  if (FSys::is_directory(json_path)) l_json_path /= "model_config.json";
  DOODLE_CHICK(FSys::exists(l_json_path), "kimodo_model_config json 文件不存在: {}", l_json_path.string());
  *this                    = nlohmann::json::parse(FSys::ifstream{l_json_path}).get<kimodo_model_config>();
  denoiser_root_path_      = l_json_path.parent_path() / "root";
  denoiser_body_path_      = l_json_path.parent_path() / "body";
  text_encoder_model_path_ = l_json_path.parent_path() / "text_encoder" / "model.onnx";
  tokenizer_json_path_     = l_json_path.parent_path() / "text_encoder" / "tokenizer.json";
  skeleton_dir_            = l_json_path.parent_path() / "skeleton";
  stats_path_              = l_json_path.parent_path() / "stats";
}

// ======================================================================
// load
// ======================================================================
void kimodo::load(std::shared_ptr<kimodo_model_config> config) {
  DOODLE_CHICK(config != nullptr, "kimodo_model_config 为空");
  SPDLOG_INFO("kimodo::load 开始...");

  llm_dim_  = config->llm_dim_;
  fps_      = static_cast<float>(config->fps_);

  // ---- Step 1: 初始化骨骼 ----
  skeleton_ = skeleton_base::create_soma_skeleton_30(config->skeleton_dir_);
  DOODLE_CHICK(skeleton_ != nullptr && skeleton_->is_valid(), "SOMASkeleton30 加载失败");

  // ---- Step 2: 初始化运动表示 ----
  motion_rep_ = std::make_shared<kimodo_motion_rep>(skeleton_, config);
  DOODLE_CHICK(motion_rep_ != nullptr && motion_rep_->motion_rep_dim() > 0, "kimodo_motion_rep 初始化失败");

  SPDLOG_INFO(
      "kimodo: motion_rep 初始化完成: dim={}, global_root_dim={}, body_dim={}", motion_rep_->motion_rep_dim(),
      motion_rep_->global_root_dim(), motion_rep_->body_dim()
  );

  // ---- Step 3: 初始化扩散过程 ----
  diffusion_.init(config);
  sampler_.set_diffusion(diffusion_);
  SPDLOG_INFO("kimodo: diffusion 初始化完成: num_base_steps={}", config->num_base_steps_);

  // ---- Step 4: 初始化去噪器（含 CFG 包装） ----
  denoiser_.load(config, motion_rep_);
  SPDLOG_INFO("kimodo: denoiser (CFG) 加载完成");

  // ---- Step 5: 初始化文本编码器 ----
  text_encoder_ = std::make_shared<LLM2Vec>(config->text_encoder_model_path_, config->tokenizer_json_path_);
  DOODLE_CHICK(text_encoder_ != nullptr, "LLM2Vec 初始化失败");
  SPDLOG_INFO("kimodo: text_encoder 初始化完成");

  text_encoder_->load_onnx();
  denoiser_.load_onnx();

  SPDLOG_INFO("kimodo::load 完成");
}

// ======================================================================
// encode_texts: 文本编码（对应 Python _generate 中的 text_encoder 调用）
// ======================================================================
kimodo::text_encoding_result kimodo::encode_texts(const std::vector<std::string>& texts) {
  const Eigen::Index B = static_cast<Eigen::Index>(texts.size());

  // ---- 对每个文本调用 LLM2Vec 获取 pooled embedding ----
  // Python LLM2VecEncoder 返回 [B, 1, D] 的单个 token 嵌入
  // C++ LLM2Vec 对每个文本返回 std::vector<float> [D]
  MatrixXfRow text_feat(B, llm_dim_);
  text_feat.setZero();

  for (Eigen::Index b = 0; b < B; ++b) {
    const auto& txt = texts[static_cast<std::size_t>(b)];

    if (txt.empty()) {
      // 空文本 → 全零（对应 Python: text_feat[empty_text_mask] = 0）
      text_feat.row(b).setZero();
    } else {
      // LLM2Vec 使用空 instruction（对应 Python: text_encoder("", text)）
      auto emb = (*text_encoder_)("", txt);
      if (emb.empty()) {
        SPDLOG_WARN("LLM2Vec 对文本 '{}' 返回空嵌入，使用零向量", txt);
        text_feat.row(b).setZero();
      } else {
        DOODLE_CHICK(
            static_cast<Eigen::Index>(emb.size()) == llm_dim_, "LLM2Vec 输出维度 {} 不匹配预期 llm_dim {}", emb.size(),
            llm_dim_
        );
        for (Eigen::Index d = 0; d < llm_dim_; ++d) {
          text_feat(b, d) = emb[static_cast<std::size_t>(d)];
        }
      }
    }
  }

  // ---- 创建 text_pad_mask: [B, 1] 全 true（Python: lengths = [1, 1, ...]） ----
  MatrixXbRow text_pad_mask(B, 1);
  text_pad_mask.setConstant(true);

  SPDLOG_INFO("kimodo: 编码 {} 个文本, text_feat shape [{}x{}]", B, text_feat.rows(), text_feat.cols());

  return {std::move(text_feat), std::move(text_pad_mask)};
}

// ======================================================================
// denoising_step: 单步去噪（对应 Python denoising_step）
// ======================================================================
MatrixXfRow kimodo::denoising_step(
    const MatrixXfRow& motion, const MatrixXbRow& pad_mask, const MatrixXfRow& text_feat,
    const MatrixXbRow& text_pad_mask, std::int64_t t, const std::vector<int64_t>& map_tensor,
    const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask, const MatrixXfRow& observed_motion,
    const std::vector<float>& cfg_weight, cfg_type cfg_type_val
) {
  // ---- 获取映射后的时间步（space_timesteps + calc_diffusion_vars 已在 generate_internal 中预计算） ----
  const std::int64_t t_map      = map_tensor[static_cast<std::size_t>(t)];

  // ---- 创建 timesteps 向量（所有 batch 元素使用相同的 t_map） ----
  const Eigen::Index batch_size = pad_mask.rows();
  std::vector<std::int64_t> timesteps_vec(static_cast<std::size_t>(batch_size), t_map);

  // ---- CFG 去噪器推理 ----
  // Python: pred_clean = self.denoiser(cfg_weight, motion, pad_mask, text_feat,
  //                                      text_pad_mask, t_map, first_heading_angle,
  //                                      motion_mask, observed_motion, cfg_type=cfg_type)
  MatrixXfRow pred_clean = denoiser_.forward(
      cfg_weight, motion, pad_mask, text_feat, text_pad_mask, timesteps_vec, first_heading_angle, motion_mask,
      observed_motion, cfg_type_val
  );

  // ---- DDIM 采样 ----
  // Python: x_tm1 = self.sampler(use_timesteps, motion, pred_clean, t)
  MatrixXfRow x_tm1 = sampler_.step(motion, pred_clean, t);

  return x_tm1;
}

// ======================================================================
// generate_internal: 完整去噪循环（对应 Python _generate）
// ======================================================================
MatrixXfRow kimodo::generate_internal(
    const std::vector<std::string>& texts, std::int64_t max_frames, std::int64_t num_denoising_steps,
    const MatrixXbRow& pad_mask, const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
    const MatrixXfRow& observed_motion, const std::vector<float>& cfg_weight, cfg_type cfg_type_val
) {
  const Eigen::Index batch_size = pad_mask.rows();
  const std::int64_t D          = motion_rep_->motion_rep_dim();
  const Eigen::Index total      = batch_size * max_frames;

  DOODLE_CHICK(
      static_cast<Eigen::Index>(texts.size()) == batch_size, "文本数 {} 不匹配 batch_size {}", texts.size(), batch_size
  );

  // ---- Step 1: 文本编码 ----
  auto [text_feat, text_pad_mask] = encode_texts(texts);
  // text_feat: [B, D] → 作为 [B*1, D] 使用（max_text_len=1）
  // text_pad_mask: [B, 1]

  // ---- Step 2: motion_mask 类型转换（bool → float） ----
  // Python: if motion_mask is not None:
  //            if motion_mask.dtype == torch.bool: motion_mask = 1 * motion_mask
  MatrixXfRow motion_mask_f;
  if (motion_mask.size() > 0) {
    motion_mask_f = motion_mask;
  }

  // ---- Step 3: 初始化噪声 ----
  // Python: cur_mot = torch.randn([B, T, D])
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dist(0.0f, 1.0f);

  MatrixXfRow cur_mot(total, D);
  for (Eigen::Index i = 0; i < cur_mot.size(); ++i) {
    cur_mot(i) = dist(gen);
  }

  // ---- Step 4: 预计算扩散时间步（仅一次，避免每步重复 space_timesteps + calc_diffusion_vars） ----
  auto [use_timesteps, map_tensor] = diffusion_.space_timesteps(num_denoising_steps);
  diffusion_.calc_diffusion_vars(use_timesteps);

  // ---- Step 5: 去噪循环 ----
  // Python: for i in reversed(range(num_denoising_steps)):
  SPDLOG_INFO("kimodo: 开始去噪循环, num_denoising_steps={}", num_denoising_steps);

  for (std::int64_t i = num_denoising_steps - 1; i >= 0; --i) {
    auto start_time = std::chrono::high_resolution_clock::now();
    cur_mot         = denoising_step(
        cur_mot, pad_mask, text_feat, text_pad_mask, i, map_tensor, first_heading_angle, motion_mask_f, observed_motion,
        cfg_weight, cfg_type_val
    );
    SPDLOG_INFO("kimodo: 去噪步 {} 完成 time {:%S}", i, std::chrono::high_resolution_clock::now() - start_time);
  }

  SPDLOG_INFO("kimodo: 去噪循环完成");
  return cur_mot;
}

// ======================================================================
// prepare_transition: 准备段间过渡
// ======================================================================
kimodo::transition_prep_result kimodo::prepare_transition(
    std::vector<MatrixXfRow>& generated_motions, MatrixXfRow& prev_latest_frames, std::int64_t prev_nb_transition,
    MatrixXfRow& observed_motion, MatrixXfRow& motion_mask_f, std::int64_t& num_frame, std::int64_t nb_transition
) {
  const std::int64_t D          = motion_rep_->motion_rep_dim();

  // 取出上一段末尾过渡帧
  MatrixXfRow& last_motion      = generated_motions.back();
  const std::int64_t last_T     = last_motion.rows();
  const std::int64_t new_last_T = last_T - prev_nb_transition;
  DOODLE_CHICK(new_last_T >= 0, "上一段帧数 {} 不足过渡帧 {}", last_T, prev_nb_transition);

  prev_latest_frames        = last_motion.bottomRows(prev_nb_transition).eval();
  last_motion               = last_motion.topRows(new_last_T).eval();

  // 解码过渡帧，获取关节数据用于构建约束
  motion_output last_output = motion_rep_->decode(prev_latest_frames, false, 1, prev_nb_transition);

  // 提取 smooth_root_2d 首帧（新段起点）
  transition_prep_result result;
  result.prev_smooth_root_2d.resize(1, 2);
  result.prev_smooth_root_2d(0, 0) = last_output.smooth_root_pos(0, 0);  // x
  result.prev_smooth_root_2d(0, 1) = last_output.smooth_root_pos(0, 2);  // z

  // 构建过渡 frame_indices [0, 1, ..., nb_transition-1]
  Eigen::VectorXi trans_indices(nb_transition);
  for (std::int64_t i = 0; i < nb_transition; ++i) trans_indices(static_cast<Eigen::Index>(i)) = static_cast<int>(i);

  // smooth_root_2d [nb_transition, 2]
  MatrixXfRow trans_smooth_root_2d(nb_transition, 2);
  for (std::int64_t i = 0; i < nb_transition; ++i) {
    trans_smooth_root_2d(i, 0) = last_output.smooth_root_pos(i, 0);
    trans_smooth_root_2d(i, 1) = last_output.smooth_root_pos(i, 2);
  }

  // 过渡约束：全身 + 末端执行器
  std::vector<kimodo_motion_rep::constraint_dicts> trans_dicts(1);

  auto fb = std::make_shared<fullbody_constraint_set>(
      skeleton_, trans_indices, last_output.posed_joints, last_output.global_rot_mats, trans_smooth_root_2d
  );
  fb->update_constraints(trans_dicts[0].data_dict, trans_dicts[0].index_dict);

  auto ee = std::make_shared<end_effector_constraint_set>(
      skeleton_, trans_indices, last_output.posed_joints, last_output.global_rot_mats, trans_smooth_root_2d,
      std::vector<std::string>{"LeftHand", "RightHand", "LeftFoot", "RightFoot"}
  );
  ee->update_constraints(trans_dicts[0].data_dict, trans_dicts[0].index_dict);

  // 保存过渡约束，供后处理使用
  result.trans_constraints.push_back(fb);
  result.trans_constraints.push_back(ee);

  Eigen::VectorXi trans_lengths(1);
  trans_lengths(0) = static_cast<int>(nb_transition);

  auto [obs_trans, mask_trans] =
      motion_rep_->create_conditions_from_constraints_batched(trans_dicts, trans_lengths, false);

  // 拼接: [transition_obs, segment_obs]
  const std::int64_t total_T = nb_transition + num_frame;
  MatrixXfRow combined_obs(total_T, D);
  MatrixXfRow combined_mask(total_T, D);
  combined_obs.setZero();
  combined_mask.setZero();

  combined_obs.topRows(nb_transition)  = obs_trans;
  combined_mask.topRows(nb_transition) = mask_trans.cast<float>();

  if (observed_motion.size() > 0) {
    combined_obs.bottomRows(num_frame)  = observed_motion;
    combined_mask.bottomRows(num_frame) = motion_mask_f;
  }

  // 平移到新段起点（原点）
  MatrixXfRow neg_trans(1, 2);
  neg_trans(0, 0)         = -result.prev_smooth_root_2d(0, 0);
  neg_trans(0, 1)         = -result.prev_smooth_root_2d(0, 1);
  combined_obs            = motion_rep_->translate_2d(combined_obs, neg_trans, 1, total_T);
  combined_obs            = combined_obs.cwiseProduct(combined_mask);

  observed_motion         = std::move(combined_obs);
  motion_mask_f           = std::move(combined_mask);
  num_frame               = total_T;

  // 从上段末尾计算朝向角
  MatrixXfRow heading_mat = compute_heading_angle(last_output.posed_joints, *skeleton_, 1, prev_nb_transition);
  result.heading_val      = heading_mat(0, 0);

  return result;
}

// ======================================================================
// generate: 多段顺序生成（对应 Python _multiprompt）
// ======================================================================
motion_output kimodo::generate(const generate_arg& segments) {
  DOODLE_CHICK(is_valid(), "kimodo 未加载或加载失败");
  DOODLE_CHICK(!segments.segments_.empty(), "segments 不能为空");

  /// 已生成的各段运动（motion_rep 特征，未标准化），每个 [1*T_i, D]，用于过渡混合
  std::vector<MatrixXfRow> generated_motions;
  /// 上一段末尾过渡帧（用于混合），[1*nb_transition, D]
  MatrixXfRow prev_latest_frames;

  for (auto is_first = true; const auto& seg : segments.segments_) {
    std::int64_t num_frame           = seg.num_frames_;
    const std::int64_t nb_transition = is_first ? 0 : seg.num_transition_frames_;

    DOODLE_CHICK(!(!is_first && nb_transition < 1), "num_transition_frames 必须 >= 1, 实际: {}", nb_transition);

    // ====================================================================
    // 构建段约束 → observed_motion / motion_mask
    // ====================================================================
    MatrixXfRow observed_motion, motion_mask_f;

    Eigen::VectorXi seg_lengths(1);
    seg_lengths(0) = static_cast<int>(num_frame);
    if (!seg.constraints_.empty()) {
      std::vector<kimodo_motion_rep::constraint_dicts> seg_dicts(1);
      for (const auto& c : seg.constraints_) {
        c->update_constraints(seg_dicts[0].data_dict, seg_dicts[0].index_dict);
      }
      auto [obs, mask] = motion_rep_->create_conditions_from_constraints_batched(
          seg_dicts, seg_lengths, false /*to_normalize*/
      );
      observed_motion = std::move(obs);
      motion_mask_f   = mask.cast<float>();
    }

    // ====================================================================
    // 过渡逻辑（非首段）
    // ====================================================================
    MatrixXfRow prev_smooth_root_2d;  // [1, 2]
    float heading_val = segments.first_heading_angle_;
    std::vector<constraint_set_ptr> trans_constraints;  // 过渡约束，供后处理使用

    if (!is_first) {
      auto trans_prep = prepare_transition(
          generated_motions, prev_latest_frames, seg.num_transition_frames_, observed_motion, motion_mask_f, num_frame,
          nb_transition
      );
      prev_smooth_root_2d = std::move(trans_prep.prev_smooth_root_2d);
      heading_val         = trans_prep.heading_val;
      trans_constraints   = std::move(trans_prep.trans_constraints);
    }

    // ====================================================================
    // 标准化 → 生成 → 反标准化
    // ====================================================================
    if (observed_motion.size() > 0) observed_motion = motion_rep_->normalize(observed_motion);

    Eigen::VectorXi lengths_vec(1);
    lengths_vec(0)                 = static_cast<int>(num_frame);
    MatrixXbRow pad_mask           = length_to_mask(lengths_vec, num_frame);

    std::vector<float> heading     = {heading_val};
    std::vector<std::string> texts = {seg.text_};

    MatrixXfRow motion             = generate_internal(
        texts, num_frame, seg.num_denoising_steps_, pad_mask, heading, motion_mask_f, observed_motion, seg.cfg_weight_,
        seg.cfg_type_
    );

    motion = motion_rep_->unnormalize(motion);

    // ====================================================================
    // 逐段后处理 / 过渡混合（对应 Python _multiprompt）
    // ====================================================================
    if (!is_first) {
      // 平移到原始位置
      motion                                 = motion_rep_->translate_2d(motion, prev_smooth_root_2d, 1, num_frame);
      const std::int64_t total_T             = num_frame;  // nb_transition + original_num_frame

      // 后处理：解码完整 transition+segment → 合并约束 → 后处理 → re-encode
      motion_output seg_output               = motion_rep_->decode(motion, false, 1, total_T);

      std::vector<constraint_set_ptr> merged = trans_constraints;
      for (const auto& c : seg.constraints_) {
        // crop_move: 将约束裁剪到 [0, num_frames) 范围
        merged.push_back(c->crop_move(-nb_transition, seg.num_frames_));
      }

      SPDLOG_INFO(
          "kimodo: 段 {} 后处理 (含过渡), total_T={}, nb_transition={}, root_margin={}", generated_motions.size(),
          total_T, nb_transition, seg.root_margin_
      );
      MatrixXfRow contacts_float = seg_output.foot_contacts.cast<float>();
      auto pp_result             = post_process_motion(
          seg_output.local_rot_mats, seg_output.root_positions, contacts_float, *skeleton_, 1, total_T, merged, 0.5f,
          seg.root_margin_
      );
      seg_output.local_rot_mats  = std::move(pp_result.local_rot_mats);
      seg_output.root_positions  = std::move(pp_result.root_positions);
      seg_output.posed_joints    = std::move(pp_result.posed_joints);
      seg_output.global_rot_mats = std::move(pp_result.global_rot_mats);

      // Re-encode 回 motion_rep 特征
      Eigen::VectorXi encode_lengths(1);
      encode_lengths(0) = static_cast<int>(total_T);
      motion =
          motion_rep_->encode(seg_output.local_rot_mats, seg_output.root_positions, false, 1, total_T, encode_lengths);

      generated_motions.push_back(std::move(motion));
    } else {
      // 首段后处理：解码 → 后处理 → re-encode
      motion_output seg_output = motion_rep_->decode(motion, false, 1, num_frame);

      SPDLOG_INFO("kimodo: 首段后处理, num_frame={}, root_margin={}", num_frame, seg.root_margin_);
      MatrixXfRow contacts_float = seg_output.foot_contacts.cast<float>();
      auto pp_result             = post_process_motion(
          seg_output.local_rot_mats, seg_output.root_positions, contacts_float, *skeleton_, 1, num_frame,
          seg.constraints_, 0.5f, seg.root_margin_
      );
      seg_output.local_rot_mats  = std::move(pp_result.local_rot_mats);
      seg_output.root_positions  = std::move(pp_result.root_positions);
      seg_output.posed_joints    = std::move(pp_result.posed_joints);
      seg_output.global_rot_mats = std::move(pp_result.global_rot_mats);

      Eigen::VectorXi encode_lengths(1);
      encode_lengths(0) = static_cast<int>(num_frame);
      motion            = motion_rep_->encode(
          seg_output.local_rot_mats, seg_output.root_positions, false, 1, num_frame, encode_lengths
      );

      generated_motions.push_back(std::move(motion));
    }

    is_first = false;
  }

  // ======================================================================
  // 拼接所有段 → 最终解码（对应 Python torch.cat + inverse）
  // ======================================================================
  const std::int64_t D      = motion_rep_->motion_rep_dim();
  std::int64_t total_frames = 0;
  for (const auto& m : generated_motions) total_frames += m.rows();

  MatrixXfRow all_motion(total_frames, D);
  std::int64_t offset = 0;
  for (const auto& m : generated_motions) {
    all_motion.middleRows(offset, m.rows()) = m;
    offset += m.rows();
  }

  motion_output output = motion_rep_->decode(all_motion, false, 1, total_frames);
  SPDLOG_INFO("kimodo::generate (multiprompt) 完成: {} 段, 总帧数 {}", segments.segments_.size(), total_frames);
  // 最终重定向 为 arg.skeleton_ 层级
  if (segments.skeleton_ == nullptr) return output;
  DOODLE_CHICK(segments.skeleton_->is_valid(), "generate_arg.skeleton_ 无效，无法进行最终重定向");
  DOODLE_CHICK(
      segments.skeleton_->nbjoints_ == skeleton_->nbjoints_, "重定向骨骼关节数 {} 与模型骨骼 {} 不匹配",
      segments.skeleton_->nbjoints_, skeleton_->nbjoints_
  );
  MatrixXfRow contacts_float = output.foot_contacts.cast<float>();
  auto pp_result             = post_process_motion(
      output.local_rot_mats, output.root_positions, contacts_float, *segments.skeleton_, 1, total_frames, {}, 0.5f, 0.0f
  );
  output.local_rot_mats  = std::move(pp_result.local_rot_mats);
  output.root_positions  = std::move(pp_result.root_positions);
  output.posed_joints    = std::move(pp_result.posed_joints);
  output.global_rot_mats = std::move(pp_result.global_rot_mats);
  // 最后将骨骼位置添加到 root_positions 中
  output.root_positions.rowwise() += segments.root_trajectory_;
  return output;
}

}  // namespace doodle::ai
