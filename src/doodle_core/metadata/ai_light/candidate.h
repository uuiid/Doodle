//
// AI 灯光：候选（ai_light_candidate）
//
// 每轮 6 个候选，每个候选一行。这一张表同时承担了方案 §10.2 里 ai_light_rig 的职责 ——
// rig 是候选的属性而不是独立实体，拆开后「rig 属于哪一轮哪个候选」还要靠关联表，纯属多余。
//
// §6.6 护栏在 schema 层面的落点：同轮 6 个候选的**灯集合与角色必须完全一致**，
// 只有 variant_axis 指定的参数不同。否则指标差异会被灯数差异淹没，
// 美术的选择无法归因到轴，搜索梯度就失效了。校验由规划层负责，本表只负责存。
//
// 「每会话至多一个定稿候选」由 accepted_session_id 上的普通唯一索引在库层强制
// （为什么不用 partial index，见该字段的注释）。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

/// 单个候选的渲染状态。与 ai_light_iteration 的 state 是两层：
/// iteration.state 描述整轮，candidate.render_status 描述这一张图。
enum class candidate_render_status {
  pending,
  rendered,
  failed,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    candidate_render_status, {
                                 {candidate_render_status::pending, "pending"},
                                 {candidate_render_status::rendered, "rendered"},
                                 {candidate_render_status::failed, "failed"},
                             }
)

/// 单个维度上「落点 vs 目标区间」的记录。
/// 这是评判层的原始材料：美术看到的不是「分数 0.72」，而是「p50 落在 0.09，目标 [0.10,0.18]」。
struct DOODLE_CORE_API metric_landing_t {
  /// 维度路径，如 "luma_percentiles.p50" / "white_balance_kelvin"
  std::string dimension_{};
  double achieved_{0.0};
  interval_t target_{};
  bool in_interval_{false};
  /// 到目标区间的归一化距离，0 = 落在区间内
  double distance_{0.0};

  friend void to_json(nlohmann::json& j, const metric_landing_t& p) {
    j["dimension"]   = p.dimension_;
    j["achieved"]    = p.achieved_;
    j["target"]      = p.target_;
    j["in_interval"] = p.in_interval_;
    j["distance"]    = p.distance_;
  }
  friend void from_json(const nlohmann::json& j, metric_landing_t& p) {
    if (j.contains("dimension")) j.at("dimension").get_to(p.dimension_);
    if (j.contains("achieved")) j.at("achieved").get_to(p.achieved_);
    if (j.contains("target")) j.at("target").get_to(p.target_);
    if (j.contains("in_interval")) j.at("in_interval").get_to(p.in_interval_);
    if (j.contains("distance")) j.at("distance").get_to(p.distance_);
  }
};

/// 候选渲染图在主指标空间（**显示空间**，已 tonemap）的实测指标。
/// 与 reference_image_stats_t 用同一批标量形状 —— 两者要能逐维比较。
struct DOODLE_CORE_API candidate_metrics_t {
  std::string space_{"display_srgb"};
  luma_percentiles_value_t luma_percentiles_{};
  std::vector<image_palette_entry_t> palette_{};
  double contrast_{0.0};
  double white_balance_kelvin_{0.0};
  clip_rate_value_t clip_rate_{};

  /// 逐维落点
  std::vector<metric_landing_t> landings_{};
  /// 落在目标区间内的维度占比（按 ownership 与 confidence 加权）
  double satisfaction_{0.0};
  /// 维度名 → 归一化距离，便于排序与对比
  std::map<std::string, double> dimension_distances_{};

  friend void to_json(nlohmann::json& j, const candidate_metrics_t& p) {
    j["space"]                = p.space_;
    j["luma_percentiles"]     = p.luma_percentiles_;
    j["palette"]              = p.palette_;
    j["contrast"]             = p.contrast_;
    j["white_balance_kelvin"] = p.white_balance_kelvin_;
    j["clip_rate"]            = p.clip_rate_;
    j["landings"]             = p.landings_;
    j["satisfaction"]         = p.satisfaction_;
    j["dimension_distances"]  = p.dimension_distances_;
  }
  friend void from_json(const nlohmann::json& j, candidate_metrics_t& p) {
    if (j.contains("space")) j.at("space").get_to(p.space_);
    if (j.contains("luma_percentiles")) j.at("luma_percentiles").get_to(p.luma_percentiles_);
    if (j.contains("palette")) j.at("palette").get_to(p.palette_);
    if (j.contains("contrast")) j.at("contrast").get_to(p.contrast_);
    if (j.contains("white_balance_kelvin")) j.at("white_balance_kelvin").get_to(p.white_balance_kelvin_);
    if (j.contains("clip_rate")) j.at("clip_rate").get_to(p.clip_rate_);
    if (j.contains("landings")) j.at("landings").get_to(p.landings_);
    if (j.contains("satisfaction")) j.at("satisfaction").get_to(p.satisfaction_);
    if (j.contains("dimension_distances")) j.at("dimension_distances").get_to(p.dimension_distances_);
  }
};

/// VLM 判官的输出。**只是决策辅助**（§8.4）：人工判断是唯一收敛判据。
struct DOODLE_CORE_API vlm_delta_t {
  std::string model_{};
  std::string verdict_{};
  std::vector<std::string> differences_{};
  std::string raw_text_{};

  friend void to_json(nlohmann::json& j, const vlm_delta_t& p) {
    j["model"]       = p.model_;
    j["verdict"]     = p.verdict_;
    j["differences"] = p.differences_;
    j["raw_text"]    = p.raw_text_;
  }
  friend void from_json(const nlohmann::json& j, vlm_delta_t& p) {
    if (j.contains("model")) j.at("model").get_to(p.model_);
    if (j.contains("verdict")) j.at("verdict").get_to(p.verdict_);
    if (j.contains("differences")) j.at("differences").get_to(p.differences_);
    if (j.contains("raw_text")) j.at("raw_text").get_to(p.raw_text_);
  }
};

// ---------------------------------------------------------------------------
// 表实体：ai_light_candidate
// ---------------------------------------------------------------------------
// 灯光候选
struct DOODLE_CORE_API ai_light_candidate {
  DOODLE_BASE_FIELDS()

  uuid iteration_id_{};
  /// **有意冗余**：前端按会话拉候选是最热路径，少一次 join。
  /// 插入后永不修改，所以不存在一致性风险。
  uuid session_id_{};

  /// 本轮第几个，0..5。§6.6 护栏要求同轮 6 个候选的灯集合一致。
  std::int32_t candidate_index_{0};

  // --- 变体轴：真实列而不是只躺在 rig JSON 里 ---
  // 目的是能直接统计「哪条轴最常被美术选中」，反哺候选生成策略。
  // 写入时由 sync_from_rig() 从 rig_.variant_axis_ 同步过来；DB 列是查询权威。
  variant_axis_kind variant_axis_{variant_axis_kind::none};
  double variant_value_{0.0};
  std::string variant_label_{};

  /// LightingRig IR 全量（§4.3）的 JSON 形态。
  /// 嵌套结构不是 ORM 的列类型，所以以 nlohmann::json 落库，用 get_rig() 类型化取用。
  /// 里面的 session_id / spec_id / iteration / candidate_index / variant_axis 是给工件
  /// 自描述用的副本（下发 UE、落盘回归），不参与任何查询。
  nlohmann::json rig_{};

  // --- 渲染产物 ---
  // 这里只有状态与实测数值：**路径不是列**（D44）。显示空间 PNG 与线性 EXR 的落点由
  // doodle_lib 的 candidate_render_path(ctx, *this, in_linear) 现算（= kitsu_ctx_t 的命名
  // 规则，会话 / 轮次 / 候选三个 uuid 本来就在行上），候选 JSON 里也不带服务端磁盘路径。
  candidate_render_status render_status_{candidate_render_status::pending};
  /// 实测渲染耗时（决策 17：先不设硬上限，但要记录）
  std::int64_t render_ms_{0};
  /// 实测灯数（决策 17：同上）
  std::int32_t light_count_{0};

  /// candidate_metrics_t 的 JSON 形态
  nlohmann::json metrics_{};

  /// VLM 判分（决策辅助）
  std::optional<double> vlm_score_{};
  /// vlm_delta_t 的 JSON 形态
  nlohmann::json vlm_delta_{};

  // --- 人工选择 ---
  /// 美术选中作为下一轮基准（可以继续迭代但不接受）
  bool is_selected_{false};
  /// 美术定稿接受。每会话至多一个。
  /// **不要直接赋值** —— 用 mark_accepted() / clear_accepted()，
  /// 它们会同步维护 accepted_session_id_（库层唯一性约束真正依赖的那一列）。
  bool is_accepted_{false};
  /// 定稿标记：定稿时写入 session_id_，否则保持 nil（落库为 NULL）。
  ///
  /// 为什么需要这一列：它是「每会话至多一个定稿候选」这条约束的载体。
  /// SQLite 的 UNIQUE 索引把 NULL 视为互不相等，所以这一列上的**普通**唯一索引
  /// 恰好等价于 `WHERE is_accepted = 1` 的 partial index：非定稿行全是 NULL，彼此不冲突；
  /// 定稿行的值等于 session_id，于是每会话只能有一行。
  ///
  /// 为什么不用 partial index：SQLite 禁止 partial index 的 WHERE 里出现绑定参数
  /// （"parameters prohibited in partial index WHERE clauses"），而本库的
  /// `create_unique_index().where(c(&T::col) == true)` 只会生成带 `?` 的比较式 ——
  /// 只有 `== nullptr` 能生成无参数的 `IS NULL`（entity 表的 parent_id 先例正是靠这一点）。
  uuid accepted_session_id_{};

  /// **只有 is_accepted 的候选才填**。中间候选不注册 preview_file，
  /// 免得镜头预览列表被 6×N 张候选淹没；中间候选由
  /// /api/doodle/ai/light/candidate/{uuid}/image 按命名规则算出 PNG 路径直接提供。
  uuid preview_file_id_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  // --- 类型化访问器 ---
  lighting_rig_t get_rig() const { return json_get_or_default<lighting_rig_t>(rig_); }
  void set_rig(const lighting_rig_t& in_rig) { rig_ = in_rig; }
  candidate_metrics_t get_metrics() const { return json_get_or_default<candidate_metrics_t>(metrics_); }
  void set_metrics(const candidate_metrics_t& in_metrics) { metrics_ = in_metrics; }
  vlm_delta_t get_vlm_delta() const { return json_get_or_default<vlm_delta_t>(vlm_delta_); }
  void set_vlm_delta(const vlm_delta_t& in_delta) { vlm_delta_ = in_delta; }

  /// 把 rig JSON 里的变体轴与索引同步到真实列，避免两处漂移。
  /// 调用点：候选落库前。DB 列是查询权威（用来统计「哪条轴最常被选中」）。
  void sync_from_rig() {
    if (rig_.is_null()) return;
    const auto l_rig = get_rig();
    variant_axis_    = l_rig.variant_axis_.axis_;
    variant_value_   = l_rig.variant_axis_.value_;
    variant_label_   = l_rig.variant_axis_.label_;
    candidate_index_ = l_rig.candidate_index_;
    session_id_      = l_rig.session_id_;
  }
  /// 反向：把真实列写回 rig 副本（构造 rig 时调用）
  void sync_to_rig() {
    auto l_rig             = get_rig();
    l_rig.variant_axis_    = variant_axis_t{variant_axis_, variant_value_, variant_label_};
    l_rig.candidate_index_ = candidate_index_;
    l_rig.session_id_      = session_id_;
    set_rig(l_rig);
  }

  /// 定稿。**唯一**允许置定稿状态的入口。
  /// 必须同时写 accepted_session_id_ —— 只写 is_accepted_ 会让库层的
  /// 唯一性约束形同虚设（那一列仍是 NULL，不参与索引）。
  void mark_accepted() {
    is_accepted_         = true;
    accepted_session_id_ = session_id_;
  }
  /// 撤销定稿（例如会话被重开）。与 mark_accepted 成对。
  void clear_accepted() {
    is_accepted_         = false;
    accepted_session_id_ = uuid{};
  }
  /// 本行是否是它所属会话的定稿候选（读路径；不依赖 is_accepted_ 是否被正确同步）
  bool is_accepted_candidate() const { return !accepted_session_id_.is_nil() && accepted_session_id_ == session_id_; }

  friend void to_json(nlohmann::json& j, const ai_light_candidate& p) {
    j["id"]                  = p.uuid_id_;
    j["iteration_id"]        = p.iteration_id_;
    j["session_id"]          = p.session_id_;
    j["candidate_index"]     = p.candidate_index_;
    j["variant_axis"]        = p.variant_axis_;
    j["variant_value"]       = p.variant_value_;
    j["variant_label"]       = p.variant_label_;
    j["rig"]                 = p.rig_;
    j["render_status"]       = p.render_status_;
    j["render_ms"]           = p.render_ms_;
    j["light_count"]         = p.light_count_;
    j["metrics"]             = p.metrics_;
    j["vlm_score"]           = p.vlm_score_;
    j["vlm_delta"]           = p.vlm_delta_;
    j["is_selected"]         = p.is_selected_;
    j["is_accepted"]         = p.is_accepted_;
    j["accepted_session_id"] = p.accepted_session_id_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["created_at"]          = p.created_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_candidate& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("iteration_id")) j.at("iteration_id").get_to(p.iteration_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("variant_axis")) j.at("variant_axis").get_to(p.variant_axis_);
    if (j.contains("variant_value")) j.at("variant_value").get_to(p.variant_value_);
    if (j.contains("variant_label")) j.at("variant_label").get_to(p.variant_label_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
    if (j.contains("render_status")) j.at("render_status").get_to(p.render_status_);
    if (j.contains("render_ms")) j.at("render_ms").get_to(p.render_ms_);
    if (j.contains("light_count")) j.at("light_count").get_to(p.light_count_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
    if (j.contains("vlm_score")) j.at("vlm_score").get_to(p.vlm_score_);
    if (j.contains("vlm_delta")) j.at("vlm_delta").get_to(p.vlm_delta_);
    if (j.contains("is_selected")) j.at("is_selected").get_to(p.is_selected_);
    if (j.contains("is_accepted")) j.at("is_accepted").get_to(p.is_accepted_);
    if (j.contains("accepted_session_id")) j.at("accepted_session_id").get_to(p.accepted_session_id_);
    if (j.contains("preview_file_id")) j.at("preview_file_id").get_to(p.preview_file_id_);
  }
};

}  // namespace doodle::ai_light
