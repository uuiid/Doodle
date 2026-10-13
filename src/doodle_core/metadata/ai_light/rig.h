//
// AI 灯光：灯光方案 LightingRig（IR）
//
// 对应 docs/ue_ai_lighting_plan.md §4.3。这一份结构体是**唯一权威定义**：
//   - 存进 ai_light_candidate.rig 这个 JSON 列
//   - 作为 server_task_info.command_ 里下发给 UE 工作机的载荷
//   - 落盘成工件供回归测试
// UE 插件侧（E:\Doodle\script\uePlug\Doodle）有自己的一份镜像实现，两端靠 JSON Schema 对齐，
// 不共享 C++ 代码。
//
// 枚举取值约定（重要）：本库的 ORM 把枚举按 magic_enum 的 **C++ 名** 存成 TEXT
// （见 sqlite_orm/orm/sqlite_statement.h:269-287）。所以
//   1) 枚举项名必须与 JSON 字符串一致（NLOHMANN_JSON_SERIALIZE_ENUM 里写的也是同一个串）；
//   2) 绝不重命名已有枚举项，否则存量行解析失败；新增项安全。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

/// 灯光在布光中的角色
enum class light_role {
  key,
  fill,
  rim,
  ambient,
  practical,
  bounce,
  fx,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    light_role, {
                    {light_role::key, "key"},
                    {light_role::fill, "fill"},
                    {light_role::rim, "rim"},
                    {light_role::ambient, "ambient"},
                    {light_role::practical, "practical"},
                    {light_role::bounce, "bounce"},
                    {light_role::fx, "fx"},
                }
)

/// UE 灯类型
enum class light_type {
  point,
  spot,
  rect,
  directional,
  sky,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    light_type, {
                    {light_type::point, "point"},
                    {light_type::spot, "spot"},
                    {light_type::rect, "rect"},
                    {light_type::directional, "directional"},
                    {light_type::sky, "sky"},
                }
)

/// 锚点所在空间。camera 是默认也是唯一推荐值：
/// 绝对世界坐标对 LLM 无意义（它不知道场景尺度），而「相机左前方 45°、抬高 30°、距离 3m」
/// 是电影布光的通用语言，且对场景尺寸鲁棒。执行器负责换算成世界坐标。
enum class anchor_space {
  camera,
  world,
  subject,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    anchor_space, {
                      {anchor_space::camera, "camera"},
                      {anchor_space::world, "world"},
                      {anchor_space::subject, "subject"},
                  }
)

/// 强度单位
enum class intensity_unit {
  candela,
  lumen,
  lux,
  unitless,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    intensity_unit, {
                        {intensity_unit::candela, "candela"},
                        {intensity_unit::lumen, "lumen"},
                        {intensity_unit::lux, "lux"},
                        {intensity_unit::unitless, "unitless"},
                    }
)

/// 颜色给定方式
enum class color_mode {
  temperature,
  rgb,
  filter,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    color_mode, {
                    {color_mode::temperature, "temperature"},
                    {color_mode::rgb, "rgb"},
                    {color_mode::filter, "filter"},
                }
)

/// UE Mobility。static_light 不能出现在每帧变化的动画里，执行器需要校验。
enum class light_mobility {
  static_light,
  stationary,
  movable,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    light_mobility, {
                        {light_mobility::static_light, "static_light"},
                        {light_mobility::stationary, "stationary"},
                        {light_mobility::movable, "movable"},
                    }
)

/// 灯光来源。inherited = 基线关卡里美术已布好的灯，AI 只做 modify/remove，不重建。
enum class light_origin {
  ai,
  artist,
  inherited,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    light_origin, {
                      {light_origin::ai, "ai"},
                      {light_origin::artist, "artist"},
                      {light_origin::inherited, "inherited"},
                  }
)

/// 候选沿哪条语义轴变化（判断六：把人工判断变成可用的搜索梯度）
enum class variant_axis_kind {
  none,
  key_direction,
  key_fill_ratio,
  kelvin,
  hardness,
  ambient_level,
  grading_strength,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    variant_axis_kind, {
                           {variant_axis_kind::none, "none"},
                           {variant_axis_kind::key_direction, "key_direction"},
                           {variant_axis_kind::key_fill_ratio, "key_fill_ratio"},
                           {variant_axis_kind::kelvin, "kelvin"},
                           {variant_axis_kind::hardness, "hardness"},
                           {variant_axis_kind::ambient_level, "ambient_level"},
                           {variant_axis_kind::grading_strength, "grading_strength"},
                       }
)

/// ops 的操作种类。add/modify/remove 针对单灯，其余针对全局。
enum class light_op_kind {
  add,
  modify,
  remove,
  set_grading,
  set_environment,
  set_exposure,
  set_white_balance,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    light_op_kind, {
                       {light_op_kind::add, "add"},
                       {light_op_kind::modify, "modify"},
                       {light_op_kind::remove, "remove"},
                       {light_op_kind::set_grading, "set_grading"},
                       {light_op_kind::set_environment, "set_environment"},
                       {light_op_kind::set_exposure, "set_exposure"},
                       {light_op_kind::set_white_balance, "set_white_balance"},
                   }
)

/// 三维向量。JSON 里是定长数组 [x, y, z]。
struct DOODLE_CORE_API vec3_t {
  double x_{0.0};
  double y_{0.0};
  double z_{0.0};

  friend void to_json(nlohmann::json& j, const vec3_t& p) { j = nlohmann::json::array({p.x_, p.y_, p.z_}); }
  friend void from_json(const nlohmann::json& j, vec3_t& p) {
    if (!j.is_array() || j.size() != 3)
      throw nlohmann::json::type_error::create(302, "vec3_t 需要长度为 3 的数组", &j);
    j.at(0).get_to(p.x_);
    j.at(1).get_to(p.y_);
    j.at(2).get_to(p.z_);
  }
};

/// 变换。rotation 用 [pitch, yaw, roll]，单位度。
struct DOODLE_CORE_API transform_t {
  vec3_t location_{};
  vec3_t rotation_{};

  friend void to_json(nlohmann::json& j, const transform_t& p) {
    j["location"] = p.location_;
    j["rotation"] = p.rotation_;
  }
  friend void from_json(const nlohmann::json& j, transform_t& p) {
    if (j.contains("location")) j.at("location").get_to(p.location_);
    if (j.contains("rotation")) j.at("rotation").get_to(p.rotation_);
  }
};

struct DOODLE_CORE_API intensity_t {
  intensity_unit unit_{intensity_unit::candela};
  double value_{0.0};

  friend void to_json(nlohmann::json& j, const intensity_t& p) {
    j["unit"]  = p.unit_;
    j["value"] = p.value_;
  }
  friend void from_json(const nlohmann::json& j, intensity_t& p) {
    if (j.contains("unit")) j.at("unit").get_to(p.unit_);
    if (j.contains("value")) j.at("value").get_to(p.value_);
  }
};

struct DOODLE_CORE_API color_t {
  color_mode mode_{color_mode::temperature};
  double kelvin_{5600.0};
  vec3_t rgb_{};
  std::string filter_{};

  friend void to_json(nlohmann::json& j, const color_t& p) {
    j["mode"] = p.mode_;
    switch (p.mode_) {
      case color_mode::temperature: j["kelvin"] = p.kelvin_; break;
      case color_mode::rgb:         j["rgb"]    = p.rgb_; break;
      case color_mode::filter:      j["filter"] = p.filter_; break;
    }
  }
  friend void from_json(const nlohmann::json& j, color_t& p) {
    if (j.contains("mode")) j.at("mode").get_to(p.mode_);
    if (j.contains("kelvin")) j.at("kelvin").get_to(p.kelvin_);
    if (j.contains("rgb")) j.at("rgb").get_to(p.rgb_);
    if (j.contains("filter")) j.at("filter").get_to(p.filter_);
  }
};

/// 锚点：灯「挂」在谁身上。target 是语义 id（不是 UE actor 名）。
struct DOODLE_CORE_API anchor_t {
  anchor_space space_{anchor_space::camera};
  std::string target_{};   ///< 主体语义 id，如 char_Ch220B；空 = 整个场景
  std::string look_at_{};  ///< 瞄准点，如 subject_head

  friend void to_json(nlohmann::json& j, const anchor_t& p) {
    j["space"] = p.space_;
    if (!p.target_.empty()) j["target"] = p.target_;
    if (!p.look_at_.empty()) j["look_at"] = p.look_at_;
  }
  friend void from_json(const nlohmann::json& j, anchor_t& p) {
    if (j.contains("space")) j.at("space").get_to(p.space_);
    if (j.contains("target")) j.at("target").get_to(p.target_);
    if (j.contains("look_at")) j.at("look_at").get_to(p.look_at_);
  }
};

/// 相对锚点的球坐标偏移
struct DOODLE_CORE_API offset_t {
  double azimuth_deg_{0.0};
  double elevation_deg_{0.0};
  double distance_m_{3.0};

  friend void to_json(nlohmann::json& j, const offset_t& p) {
    j["azimuth_deg"]   = p.azimuth_deg_;
    j["elevation_deg"] = p.elevation_deg_;
    j["distance_m"]    = p.distance_m_;
  }
  friend void from_json(const nlohmann::json& j, offset_t& p) {
    if (j.contains("azimuth_deg")) j.at("azimuth_deg").get_to(p.azimuth_deg_);
    if (j.contains("elevation_deg")) j.at("elevation_deg").get_to(p.elevation_deg_);
    if (j.contains("distance_m")) j.at("distance_m").get_to(p.distance_m_);
  }
};

struct DOODLE_CORE_API shape_t {
  double source_radius_cm_{0.0};
  std::string ies_{};
  std::optional<double> inner_cone_deg_{};
  std::optional<double> outer_cone_deg_{};

  friend void to_json(nlohmann::json& j, const shape_t& p) {
    j["source_radius_cm"] = p.source_radius_cm_;
    j["ies"]              = p.ies_;
    if (p.inner_cone_deg_) j["inner_cone_deg"] = *p.inner_cone_deg_;
    if (p.outer_cone_deg_) j["outer_cone_deg"] = *p.outer_cone_deg_;
  }
  friend void from_json(const nlohmann::json& j, shape_t& p) {
    if (j.contains("source_radius_cm")) j.at("source_radius_cm").get_to(p.source_radius_cm_);
    if (j.contains("ies") && j.at("ies").is_string()) j.at("ies").get_to(p.ies_);
    if (j.contains("inner_cone_deg") && !j.at("inner_cone_deg").is_null())
      j.at("inner_cone_deg").get_to(p.inner_cone_deg_.emplace());
    if (j.contains("outer_cone_deg") && !j.at("outer_cone_deg").is_null())
      j.at("outer_cone_deg").get_to(p.outer_cone_deg_.emplace());
  }
};

struct DOODLE_CORE_API shadows_t {
  bool cast_{true};
  double softness_{0.5};

  friend void to_json(nlohmann::json& j, const shadows_t& p) {
    j["cast"]     = p.cast_;
    j["softness"] = p.softness_;
  }
  friend void from_json(const nlohmann::json& j, shadows_t& p) {
    if (j.contains("cast")) j.at("cast").get_to(p.cast_);
    if (j.contains("softness")) j.at("softness").get_to(p.softness_);
  }
};

/// 强度动画关键帧。只有 movable 灯能带动画。
struct DOODLE_CORE_API anim_key_t {
  std::int32_t frame_{0};
  double intensity_{0.0};

  friend void to_json(nlohmann::json& j, const anim_key_t& p) {
    j["frame"]     = p.frame_;
    j["intensity"] = p.intensity_;
  }
  friend void from_json(const nlohmann::json& j, anim_key_t& p) {
    if (j.contains("frame")) j.at("frame").get_to(p.frame_);
    if (j.contains("intensity")) j.at("intensity").get_to(p.intensity_);
  }
};

/// 单盏灯的完整描述
struct DOODLE_CORE_API light_t {
  /// 稳定语义 id，绝不用 UE actor 名（美术会重命名/复制/删除 actor）
  std::string id_{};
  light_role role_{light_role::key};
  light_type type_{light_type::spot};
  anchor_t anchor_{};
  offset_t offset_{};
  intensity_t intensity_{};
  color_t color_{};
  shape_t shape_{};
  shadows_t shadows_{};
  /// 通道 [r, g, b]，1/0 表示启用。项目约定：主光走 Channel1 → [0,1,0]
  std::vector<std::int32_t> channels_{0, 1, 0};
  light_mobility mobility_{light_mobility::movable};
  std::vector<anim_key_t> anim_{};
  light_origin origin_{light_origin::ai};

  friend void to_json(nlohmann::json& j, const light_t& p) {
    j["id"]        = p.id_;
    j["role"]      = p.role_;
    j["type"]      = p.type_;
    j["anchor"]    = p.anchor_;
    j["offset"]    = p.offset_;
    j["intensity"] = p.intensity_;
    j["color"]     = p.color_;
    j["shape"]     = p.shape_;
    j["shadows"]   = p.shadows_;
    j["channels"]  = p.channels_;
    j["mobility"]  = p.mobility_;
    j["anim"]      = p.anim_;
    j["origin"]    = p.origin_;
  }
  friend void from_json(const nlohmann::json& j, light_t& p) {
    if (j.contains("id")) j.at("id").get_to(p.id_);
    if (j.contains("role")) j.at("role").get_to(p.role_);
    if (j.contains("type")) j.at("type").get_to(p.type_);
    if (j.contains("anchor")) j.at("anchor").get_to(p.anchor_);
    if (j.contains("offset")) j.at("offset").get_to(p.offset_);
    if (j.contains("intensity")) j.at("intensity").get_to(p.intensity_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("shape")) j.at("shape").get_to(p.shape_);
    if (j.contains("shadows")) j.at("shadows").get_to(p.shadows_);
    if (j.contains("channels")) j.at("channels").get_to(p.channels_);
    if (j.contains("mobility")) j.at("mobility").get_to(p.mobility_);
    if (j.contains("anim")) j.at("anim").get_to(p.anim_);
    if (j.contains("origin")) j.at("origin").get_to(p.origin_);
  }
};

/// 本候选沿哪条语义轴变化。§6.6 护栏要求同轮 6 个候选的灯集合与角色完全一致，
/// 只有轴参数不同 —— 否则指标差异会被灯数差异淹没，美术的选择无法归因到轴。
struct DOODLE_CORE_API variant_axis_t {
  variant_axis_kind axis_{variant_axis_kind::none};
  double value_{0.0};
  std::string label_{};

  friend void to_json(nlohmann::json& j, const variant_axis_t& p) {
    j["axis"]  = p.axis_;
    j["value"] = p.value_;
    j["label"] = p.label_;
  }
  friend void from_json(const nlohmann::json& j, variant_axis_t& p) {
    if (j.contains("axis")) j.at("axis").get_to(p.axis_);
    if (j.contains("value")) j.at("value").get_to(p.value_);
    if (j.contains("label")) j.at("label").get_to(p.label_);
  }
};

struct DOODLE_CORE_API provenance_t {
  std::string model_{};
  std::string prompt_hash_{};
  std::vector<std::string> reference_{};
  std::int64_t seed_{0};
  std::string tier_{};

  friend void to_json(nlohmann::json& j, const provenance_t& p) {
    j["model"]       = p.model_;
    j["prompt_hash"] = p.prompt_hash_;
    j["reference"]   = p.reference_;
    j["seed"]        = p.seed_;
    j["tier"]        = p.tier_;
  }
  friend void from_json(const nlohmann::json& j, provenance_t& p) {
    if (j.contains("model")) j.at("model").get_to(p.model_);
    if (j.contains("prompt_hash")) j.at("prompt_hash").get_to(p.prompt_hash_);
    if (j.contains("reference")) j.at("reference").get_to(p.reference_);
    if (j.contains("seed")) j.at("seed").get_to(p.seed_);
    if (j.contains("tier")) j.at("tier").get_to(p.tier_);
  }
};

struct DOODLE_CORE_API exposure_t {
  std::string method_{"manual"};
  double ev100_bias_{0.0};

  friend void to_json(nlohmann::json& j, const exposure_t& p) {
    j["method"]     = p.method_;
    j["ev100_bias"] = p.ev100_bias_;
  }
  friend void from_json(const nlohmann::json& j, exposure_t& p) {
    if (j.contains("method")) j.at("method").get_to(p.method_);
    if (j.contains("ev100_bias")) j.at("ev100_bias").get_to(p.ev100_bias_);
  }
};

struct DOODLE_CORE_API white_balance_t {
  double kelvin_{5600.0};
  double tint_{0.0};

  friend void to_json(nlohmann::json& j, const white_balance_t& p) {
    j["kelvin"] = p.kelvin_;
    j["tint"]   = p.tint_;
  }
  friend void from_json(const nlohmann::json& j, white_balance_t& p) {
    if (j.contains("kelvin")) j.at("kelvin").get_to(p.kelvin_);
    if (j.contains("tint")) j.at("tint").get_to(p.tint_);
  }
};

struct DOODLE_CORE_API environment_t {
  double sky_light_intensity_{1.0};
  double fog_density_{0.0};

  friend void to_json(nlohmann::json& j, const environment_t& p) {
    j["sky_light_intensity"] = p.sky_light_intensity_;
    j["fog_density"]         = p.fog_density_;
  }
  friend void from_json(const nlohmann::json& j, environment_t& p) {
    if (j.contains("sky_light_intensity")) j.at("sky_light_intensity").get_to(p.sky_light_intensity_);
    if (j.contains("fog_density")) j.at("fog_density").get_to(p.fog_density_);
  }
};

/// 后处理/调色。决策 8：这一块允许 AI 修改，且参考以全局调色为主，
/// 所以 grading 是达成目标的主力手段之一（§9.2 全局调色反解）。
struct DOODLE_CORE_API grading_t {
  double contrast_{1.0};
  double saturation_{1.0};
  double lift_{0.0};
  double vignette_{0.0};
  double bloom_{0.0};

  friend void to_json(nlohmann::json& j, const grading_t& p) {
    j["contrast"]   = p.contrast_;
    j["saturation"] = p.saturation_;
    j["lift"]       = p.lift_;
    j["vignette"]   = p.vignette_;
    j["bloom"]      = p.bloom_;
  }
  friend void from_json(const nlohmann::json& j, grading_t& p) {
    if (j.contains("contrast")) j.at("contrast").get_to(p.contrast_);
    if (j.contains("saturation")) j.at("saturation").get_to(p.saturation_);
    if (j.contains("lift")) j.at("lift").get_to(p.lift_);
    if (j.contains("vignette")) j.at("vignette").get_to(p.vignette_);
    if (j.contains("bloom")) j.at("bloom").get_to(p.bloom_);
  }
};

/// 全局参数（与单灯无关的部分）
struct DOODLE_CORE_API globals_t {
  exposure_t exposure_{};
  white_balance_t white_balance_{};
  environment_t environment_{};
  grading_t grading_{};

  friend void to_json(nlohmann::json& j, const globals_t& p) {
    j["exposure"]      = p.exposure_;
    j["white_balance"] = p.white_balance_;
    j["environment"]   = p.environment_;
    j["grading"]       = p.grading_;
  }
  friend void from_json(const nlohmann::json& j, globals_t& p) {
    if (j.contains("exposure")) j.at("exposure").get_to(p.exposure_);
    if (j.contains("white_balance")) j.at("white_balance").get_to(p.white_balance_);
    if (j.contains("environment")) j.at("environment").get_to(p.environment_);
    if (j.contains("grading")) j.at("grading").get_to(p.grading_);
  }
};

/// 相对基线关卡的一步操作。patch 保持 json 形态：
/// 不同 op 的 patch 结构不同，硬塞成结构体只会让新增 op 变成破坏性改动。
struct DOODLE_CORE_API light_op_t {
  light_op_kind op_{light_op_kind::add};
  std::string light_{};       ///< 目标灯的语义 id（全局 op 为空）
  nlohmann::json patch_{};    ///< 该 op 的参数
  std::string rationale_{};   ///< 为什么这么做，进日志与前端展示

  friend void to_json(nlohmann::json& j, const light_op_t& p) {
    j["op"] = p.op_;
    if (!p.light_.empty()) j["light"] = p.light_;
    if (!p.patch_.is_null()) j["patch"] = p.patch_;
    if (!p.rationale_.empty()) j["rationale"] = p.rationale_;
  }
  friend void from_json(const nlohmann::json& j, light_op_t& p) {
    if (j.contains("op")) j.at("op").get_to(p.op_);
    if (j.contains("light")) j.at("light").get_to(p.light_);
    if (j.contains("patch")) j.at("patch").get_to(p.patch_);
    if (j.contains("rationale")) j.at("rationale").get_to(p.rationale_);
  }
};

/// 灯光方案。存进 ai_light_candidate.rig 这一列，也直接作为下发 UE 的载荷。
///
/// 关于 session_id / iteration / candidate_index 的重复：
/// DB 里另有同名真实列（供查询与索引），JSON 里这一份是给工件自描述用的
/// （下发 UE、落盘回归、脱离数据库也能读懂）。写入时必须两者同源，
/// **DB 列是查询权威**，JSON 副本不参与任何查询。
/// （原先这里还有 spec_id —— Spec 表已删除，它不是 DB 列了，也不再进 JSON。）
struct DOODLE_CORE_API lighting_rig_t {
  std::string schema_version_{std::string{g_rig_schema_version}};
  uuid rig_id_{};
  uuid session_id_{};
  uuid shot_task_id_{};
  std::int32_t iteration_{0};
  std::int32_t candidate_index_{0};
  variant_axis_t variant_axis_{};
  provenance_t provenance_{};
  /// 本轮起点关卡（只读，永不修改）
  std::string baseline_level_{};
  globals_t globals_{};
  std::vector<light_t> lights_{};
  std::vector<light_op_t> ops_{};

  friend void to_json(nlohmann::json& j, const lighting_rig_t& p) {
    j["schema_version"]  = p.schema_version_;
    j["rig_id"]          = p.rig_id_;
    j["session_id"]      = p.session_id_;
    j["shot_task_id"]    = p.shot_task_id_;
    j["iteration"]       = p.iteration_;
    j["candidate_index"] = p.candidate_index_;
    j["variant_axis"]    = p.variant_axis_;
    j["provenance"]      = p.provenance_;
    j["baseline_level"]  = p.baseline_level_;
    j["globals"]         = p.globals_;
    j["lights"]          = p.lights_;
    j["ops"]             = p.ops_;
  }
  friend void from_json(const nlohmann::json& j, lighting_rig_t& p) {
    if (j.contains("schema_version")) j.at("schema_version").get_to(p.schema_version_);
    if (j.contains("rig_id")) j.at("rig_id").get_to(p.rig_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("shot_task_id")) j.at("shot_task_id").get_to(p.shot_task_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("variant_axis")) j.at("variant_axis").get_to(p.variant_axis_);
    if (j.contains("provenance")) j.at("provenance").get_to(p.provenance_);
    if (j.contains("baseline_level")) j.at("baseline_level").get_to(p.baseline_level_);
    if (j.contains("globals")) j.at("globals").get_to(p.globals_);
    if (j.contains("lights")) j.at("lights").get_to(p.lights_);
    if (j.contains("ops")) j.at("ops").get_to(p.ops_);
  }
};

}  // namespace doodle::ai_light
