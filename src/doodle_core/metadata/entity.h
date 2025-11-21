//
// Created by TD on 25-1-16.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <bitset>

namespace doodle {

enum class entity_status {
  standby,
  running,
  complete,
  canceled,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    entity_status, {{entity_status::standby, "standby"},
                    {entity_status::running, "running"},
                    {entity_status::complete, "complete"},
                    {entity_status::canceled, "canceled"}}
)
/// 解算状态
const constexpr std::bitset<2> simulation_status_cloth      = 0b01;
const constexpr std::bitset<2> simulation_status_hair       = 0b10;
const constexpr std::bitset<2> simulation_status_cloth_hair = 0b11;

struct DOODLE_CORE_API asset_instance_link {
  std::int32_t id_;
  uuid entity_id_;
  uuid asset_instance_id_;
};

struct DOODLE_CORE_API entity_link {
  DOODLE_BASE_FIELDS();
  uuid entity_in_id_;   // 指向镜头,序列等
  uuid entity_out_id_;  // 指向资产
  nlohmann::json data_;
  std::int32_t nb_occurences_{1};
  std::string label_;
  // form json
  friend void from_json(const nlohmann::json& j, entity_link& p) {
    if (j.contains("id")) j.at("id").get_to(p.id_);
    if (j.contains("entity_in_id")) j.at("entity_in_id").get_to(p.entity_in_id_);
    if (j.contains("entity_out_id")) j.at("entity_out_id").get_to(p.entity_out_id_);
    if (j.contains("data")) j.at("data").get_to(p.data_);
    if (j.contains("nb_occurences")) j.at("nb_occurences").get_to(p.nb_occurences_);
    if (j.contains("label")) j.at("label").get_to(p.label_);
    if (j.contains("asset_id")) j.at("asset_id").get_to(p.entity_out_id_);
  }
  // to json
  friend void to_json(nlohmann::json& j, const entity_link& p) {
    j["id"]            = p.uuid_id_;
    j["entity_id"]     = p.entity_in_id_;
    j["entity_out_id"] = p.entity_out_id_;
    j["data"]          = p.data_;
    j["nb_occurences"] = p.nb_occurences_;
    j["label"]         = p.label_;
    j["asset_id"]      = p.entity_out_id_;
  }
};
struct DOODLE_CORE_API entity_concept_link {
  std::int32_t id_;
  uuid entity_id_;
  uuid entity_out_id_;
};
struct DOODLE_CORE_API entity_shot_extend {
  DOODLE_BASE_FIELDS();
  uuid entity_id_;
  // to json
  friend void to_json(nlohmann::json& j, const entity_shot_extend& p) {}
  // from json
  friend void from_json(const nlohmann::json& j, entity_shot_extend& p) {}
};
struct DOODLE_CORE_API entity_asset_extend {
  DOODLE_BASE_FIELDS();

  uuid entity_id_;

  std::optional<std::int32_t> ji_shu_lie_;
  std::string deng_ji_;
  std::optional<std::int32_t> gui_dang_;
  std::string bian_hao_;
  std::string pin_yin_ming_cheng_;
  std::string ban_ben_;
  std::optional<std::int32_t> ji_du_;
  std::optional<std::int32_t> kai_shi_ji_shu_;
  std::optional<std::int32_t> chang_ci_{};

  bool is_empty() const {
    return !ji_shu_lie_ && deng_ji_.empty() && !gui_dang_ && bian_hao_.empty() && pin_yin_ming_cheng_.empty() &&
           ban_ben_.empty() && !ji_du_ && !kai_shi_ji_shu_ && !chang_ci_;
  }
  static bool has_extend_data(const nlohmann::json& j) {
    return j.contains("ji_shu_lie") || j.contains("deng_ji") || j.contains("gui_dang") || j.contains("bian_hao") ||
           j.contains("pin_yin_ming_cheng") || j.contains("ban_ben") || j.contains("ji_du") ||
           j.contains("kai_shi_ji_shu") || j.contains("chang_ci");
  }

  // to json
  friend void to_json(nlohmann::json& j, const entity_asset_extend& p) {
    j["ji_shu_lie"]         = p.ji_shu_lie_;
    j["deng_ji"]            = p.deng_ji_;
    j["gui_dang"]           = p.gui_dang_;
    j["bian_hao"]           = p.bian_hao_;
    j["pin_yin_ming_cheng"] = p.pin_yin_ming_cheng_;
    j["ban_ben"]            = p.ban_ben_;
    j["ji_du"]              = p.ji_du_;
    j["kai_shi_ji_shu"]     = p.kai_shi_ji_shu_;
    j["chang_ci"]           = p.chang_ci_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, entity_asset_extend& p) {
    if (j.contains("ji_shu_lie")) j.at("ji_shu_lie").get_to(p.ji_shu_lie_);
    if (j.contains("deng_ji")) j.at("deng_ji").get_to(p.deng_ji_);
    if (j.contains("gui_dang")) j.at("gui_dang").get_to(p.gui_dang_);
    if (j.contains("bian_hao")) j.at("bian_hao").get_to(p.bian_hao_);
    if (j.contains("pin_yin_ming_cheng")) j.at("pin_yin_ming_cheng").get_to(p.pin_yin_ming_cheng_);
    if (j.contains("ban_ben")) j.at("ban_ben").get_to(p.ban_ben_);
    if (j.contains("ji_du")) j.at("ji_du").get_to(p.ji_du_);
    if (j.contains("kai_shi_ji_shu")) j.at("kai_shi_ji_shu").get_to(p.kai_shi_ji_shu_);
    if (j.contains("chang_ci")) j.at("chang_ci").get_to(p.chang_ci_);
  }
};

struct DOODLE_CORE_API entity {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string code_;
  std::string description_;
  std::int32_t shotgun_id_;
  bool canceled_;

  std::optional<std::int32_t> nb_frames_;
  std::int32_t nb_entities_out_;
  bool is_casting_standby_;

  bool is_shared_;
  entity_status status_;

  // 外键
  /// 项目外键
  uuid project_id_;
  /// 实体类型外键
  uuid entity_type_id_;
  /// 父外键(未知)
  uuid parent_id_;
  /// 源外键(指向集数uuid)
  uuid source_id_;
  uuid preview_file_id_;

  uuid ready_for_;
  /// 创建人
  uuid created_by_;
  std::vector<uuid> entities_out;
  std::vector<uuid> entity_concept_links_;
  std::vector<uuid> instance_casting_;

  std::tuple<std::string, uuid> get_full_name() const;

  // to json
  friend void to_json(nlohmann::json& j, const entity& p) {
    j["id"]                   = p.uuid_id_;
    j["name"]                 = p.name_;
    j["code"]                 = p.code_;
    j["description"]          = p.description_;
    j["shotgun_id"]           = p.shotgun_id_;
    j["canceled"]             = p.canceled_;
    j["nb_frames"]            = p.nb_frames_;
    j["nb_entities_out"]      = p.nb_entities_out_;
    j["is_casting_standby"]   = p.is_casting_standby_;
    j["is_shared"]            = p.is_shared_;
    j["status"]               = p.status_;
    j["project_id"]           = p.project_id_;
    j["entity_type_id"]       = p.entity_type_id_;
    j["parent_id"]            = p.parent_id_;
    j["source_id"]            = p.source_id_;
    j["preview_file_id"]      = p.preview_file_id_;
    j["ready_for"]            = p.ready_for_;
    j["created_by"]           = p.created_by_;
    j["entities_out"]         = p.entities_out;
    j["entity_concept_links"] = p.entity_concept_links_;
    j["instance_casting"]     = p.instance_casting_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, entity& p) {
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("entity_type_id")) j.at("entity_type_id").get_to(p.entity_type_id_);
    if (j.contains("is_shared")) j.at("is_shared").get_to(p.is_shared_);
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("ready_for") && !j.at("ready_for").is_null()) j.at("ready_for").get_to(p.ready_for_);
    if (j.contains("canceled") && !j.at("canceled").is_null()) j.at("canceled").get_to(p.canceled_);
    if (j.contains("code") && !j.at("code").is_null()) j.at("code").get_to(p.code_);
    if (j.contains("status") && !j.at("status").is_null()) j.at("status").get_to(p.status_);
    if (j.contains("is_casting_standby")) j.at("is_casting_standby").get_to(p.is_casting_standby_);
    if (j.contains("parent_id")) j.at("parent_id").get_to(p.parent_id_);
  }
};
}  // namespace doodle