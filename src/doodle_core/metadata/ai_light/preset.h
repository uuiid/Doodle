//
// AI 灯光：预设库（ai_light_preset）
//
// 方案 §6.4：知识注入。首批由美术整理（三点布光 / 伦勃朗 / 剪影 / 夜景霓虹 …），
// 之后可以从被接受的会话里回捞（source = learned）。
//
// 检索方式：**全表扫描 + 内存余弦相似度**。sqlite 没有向量索引，
// 而首批规模是几十条，为它引入向量库是过度设计。规模真的上来了再换。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/ai_light/spec.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::ai_light {

/// 适用范围：决定这条预设在什么场景下可以被检索出来
struct DOODLE_CORE_API preset_applicable_t {
  std::vector<std::string> scene_type_{};  ///< interior | exterior | night | day ...
  std::optional<interval_t> subject_count_{};
  bool interior_{true};

  friend void to_json(nlohmann::json& j, const preset_applicable_t& p) {
    j["scene_type"] = p.scene_type_;
    if (p.subject_count_) j["subject_count"] = *p.subject_count_;
    j["interior"] = p.interior_;
  }
  friend void from_json(const nlohmann::json& j, preset_applicable_t& p) {
    if (j.contains("scene_type")) j.at("scene_type").get_to(p.scene_type_);
    if (j.contains("subject_count") && !j.at("subject_count").is_null())
      j.at("subject_count").get_to(p.subject_count_.emplace());
    if (j.contains("interior")) j.at("interior").get_to(p.interior_);
  }
};

// ---------------------------------------------------------------------------
// 表实体：ai_light_preset
// ---------------------------------------------------------------------------

struct DOODLE_CORE_API ai_light_preset {
  DOODLE_BASE_FIELDS()

  std::string name_{};
  std::vector<std::string> tags_{};

  /// 检索向量。存成 JSON 数组（TEXT）而不是 BLOB：
  /// 本库的 ORM 只为 uuid 提供了 BLOB 绑定，没有 std::vector<float> 的绑定
  /// （见 sqlite_orm/orm/sqlite_statement.h:110-140 与 :292-375）。
  /// 为省一半空间去给 ORM 加一个浮点向量绑定，不值得；几十条数据下空间无关紧要。
  nlohmann::json embedding_{};
  std::int32_t embedding_dim_{0};

  /// 一段 LightingRig（JSON 形态）。identity 字段（rig_id / session_id / shot_task_id /
  /// spec_id / iteration / candidate_index）在片段里留空，只有 globals 与 lights 有意义。
  nlohmann::json rig_fragment_{};

  /// preset_applicable_t 的 JSON 形态
  nlohmann::json applicable_{};
  /// 预设自带的目标统计，可用来在没有参考图时兜底（prompt_only 降级）
  nlohmann::json stats_target_{};

  bool enabled_{true};
  uuid created_by_{};

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_light_preset::name_},                    //
        std::pair{"tags", &ai_light_preset::tags_},                    //
        std::pair{"embedding", &ai_light_preset::embedding_},          //
        std::pair{"embedding_dim", &ai_light_preset::embedding_dim_},  //
        std::pair{"rig_fragment", &ai_light_preset::rig_fragment_},    //
        std::pair{"applicable", &ai_light_preset::applicable_},        //
        std::pair{"stats_target", &ai_light_preset::stats_target_},    //
        std::pair{"enabled", &ai_light_preset::enabled_}               //
    };
  }

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  // --- 类型化访问器 ---
  lighting_rig_t get_rig_fragment() const { return rig_fragment_.get<lighting_rig_t>(); }
  void set_rig_fragment(const lighting_rig_t& in_rig) { rig_fragment_ = in_rig; }
  preset_applicable_t get_applicable() const { return applicable_.get<preset_applicable_t>(); }
  void set_applicable(const preset_applicable_t& in_applicable) { applicable_ = in_applicable; }
  lighting_targets_t get_stats_target() const { return stats_target_.get<lighting_targets_t>(); }
  void set_stats_target(const lighting_targets_t& in_targets) { stats_target_ = in_targets; }

  friend void to_json(nlohmann::json& j, const ai_light_preset& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["tags"]          = p.tags_;
    j["embedding"]     = p.embedding_;
    j["embedding_dim"] = p.embedding_dim_;
    j["rig_fragment"]  = p.rig_fragment_;
    j["applicable"]    = p.applicable_;
    j["stats_target"]  = p.stats_target_;
    j["enabled"]       = p.enabled_;
    j["created_by"]    = p.created_by_;
    j["created_at"]    = p.created_at_;
    j["updated_at"]    = p.updated_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_preset& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("tags")) j.at("tags").get_to(p.tags_);
    if (j.contains("embedding")) j.at("embedding").get_to(p.embedding_);
    if (j.contains("embedding_dim")) j.at("embedding_dim").get_to(p.embedding_dim_);
    if (j.contains("rig_fragment")) j.at("rig_fragment").get_to(p.rig_fragment_);
    if (j.contains("applicable")) j.at("applicable").get_to(p.applicable_);
    if (j.contains("stats_target")) j.at("stats_target").get_to(p.stats_target_);
    if (j.contains("enabled")) j.at("enabled").get_to(p.enabled_);
    if (j.contains("created_by")) j.at("created_by").get_to(p.created_by_);
  }
};

}  // namespace doodle::ai_light
