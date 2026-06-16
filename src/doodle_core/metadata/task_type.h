//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct task_type {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string short_name_;
  std::string description_;
  std::string color_;
  std::int32_t priority_;
  std::string for_entity_;
  bool allow_timelog_;
  bool archived_;
  std::optional<std::int32_t> shotgun_id_;
  uuid department_id_;

  /// 原画
  constexpr static uuid get_original_painting_id() {
    // 1a26b082-8cb3-4d3c-9054-50a069ac64b2
    return {{0x1a, 0x26, 0xb0, 0x82, 0x8c, 0xb3, 0x4d, 0x3c, 0x90, 0x54, 0x50, 0xa0, 0x69, 0xac, 0x64, 0xb2}};
  }
  /// 角色
  constexpr static uuid get_character_id() {
    // 3e20ff2b-13e6-4dce-8bf2-37341b5c1f34
    return {{0x3e, 0x20, 0xff, 0x2b, 0x13, 0xe6, 0x4d, 0xce, 0x8b, 0xf2, 0x37, 0x34, 0x1b, 0x5c, 0x1f, 0x34}};
  }
  /// 地编模型
  constexpr static uuid get_ground_model_id() {
    // 13ddf60c-ed8e-4e65-85bb-57dc4207aeca
    return {{0x13, 0xdd, 0xf6, 0x0c, 0xed, 0x8e, 0x4e, 0x65, 0x85, 0xbb, 0x57, 0xdc, 0x42, 0x07, 0xae, 0xca}};
  }
  /// 绑定
  constexpr static uuid get_binding_id() {
    // 32504e3e-381c-4f36-bdeb-f73328f96f9c
    return {{0x32, 0x50, 0x4e, 0x3e, 0x38, 0x1c, 0x4f, 0x36, 0xbd, 0xeb, 0xf7, 0x33, 0x28, 0xf9, 0x6f, 0x9c}};
  }
  /// 解算资产
  constexpr static uuid get_simulation_id() {
    // da050d42-4f45-40c4-9638-cc637753d3b5
    return {{0xda, 0x05, 0x0d, 0x42, 0x4f, 0x45, 0x40, 0xc4, 0x96, 0x38, 0xcc, 0x63, 0x77, 0x53, 0xd3, 0xb5}};
  }
  /// 特效资产
  constexpr static uuid get_effect_asset_id() {
    // c0d5224e-70c3-4b2f-8f49-348d6e051029
    return {{0xc0, 0xd5, 0x22, 0x4e, 0x70, 0xc3, 0x4b, 0x2f, 0x8f, 0x49, 0x34, 0x8d, 0x6e, 0x05, 0x10, 0x29}};
  }
  /// 镜头特效
  constexpr static uuid get_shot_effect_id() {
    // a33b7371-038c-4628-93b2-6754fc4f302b
    return {{0xa3, 0x3b, 0x73, 0x71, 0x03, 0x8c, 0x46, 0x28, 0x93, 0xb2, 0x67, 0x54, 0xfc, 0x4f, 0x30, 0x2b}};
  }

  /// 灯光
  constexpr static uuid get_lighting_id() {
    // 9be21729-be9e-4914-afc4-6046ed089886
    return {{0x9b, 0xe2, 0x17, 0x29, 0xbe, 0x9e, 0x49, 0x14, 0xaf, 0xc4, 0x60, 0x46, 0xed, 0x08, 0x98, 0x86}};
  }
  /// 动画
  constexpr static uuid get_animation_id() {
    // eb7c92c8-232c-4894-8efa-c62ced44ff05
    return {{0xeb, 0x7c, 0x92, 0xc8, 0x23, 0x2c, 0x48, 0x94, 0x8e, 0xfa, 0xc6, 0x2c, 0xed, 0x44, 0xff, 0x05}};
  }
  /// 解算任务
  constexpr static uuid get_simulation_task_id() {
    // 9d71918b-cbf0-46bc-9c39-27177c9a950a
    return {{0x9d, 0x71, 0x91, 0x8b, 0xcb, 0xf0, 0x46, 0xbc, 0x9c, 0x39, 0x27, 0x17, 0x7c, 0x9a, 0x95, 0x0a}};
  }

  constexpr static std::array<task_type, 10> get_all_constant();

  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, task_type& p) {
    j.at("name").get_to(p.name_);
    j.at("short_name").get_to(p.short_name_);
    if (j.contains("description") && !j.at("description").is_null()) j.at("description").get_to(p.description_);
    j.at("color").get_to(p.color_);
    j.at("priority").get_to(p.priority_);
    j.at("for_entity").get_to(p.for_entity_);
    j.at("allow_timelog").get_to(p.allow_timelog_);
    if (j.contains("archived") && !j.at("archived").is_null()) j.at("archived").get_to(p.archived_);
    if (j.contains("shotgun_id") && !j.at("shotgun_id").is_null()) j.at("shotgun_id").get_to(p.shotgun_id_);
    j.at("department_id").get_to(p.department_id_);
  }
  // to json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const task_type& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["short_name"]    = p.short_name_;
    j["description"]   = p.description_;
    j["color"]         = p.color_;
    j["priority"]      = p.priority_;
    j["for_entity"]    = p.for_entity_;
    j["allow_timelog"] = p.allow_timelog_;
    j["archived"]      = p.archived_;
    j["shotgun_id"]    = p.shotgun_id_;
    j["department_id"] = p.department_id_;
  }
};

constexpr std::array<task_type, 10> task_type::get_all_constant() {
  // clang-format off
  return {
      task_type{.uuid_id_ = get_original_painting_id(), .name_ = "原画",    .short_name_ = "原画",      .for_entity_ = "Asset", },
      task_type{.uuid_id_ = get_character_id(),         .name_ = "角色",    .short_name_ = "角色",      .for_entity_ = "Asset", },
      task_type{.uuid_id_ = get_ground_model_id(),      .name_ = "地编模型", .short_name_ = "地编模型",  .for_entity_ = "Asset", },
      task_type{.uuid_id_ = get_binding_id(),           .name_ = "绑定",     .short_name_ = "绑定",     .for_entity_ = "Asset", },
      task_type{.uuid_id_ = get_simulation_id(),        .name_ = "解算资产", .short_name_ = "解算资产",  .for_entity_ = "Asset", },
      task_type{.uuid_id_ = get_effect_asset_id(),      .name_ = "特效资产", .short_name_= "特效资产",   .for_entity_= "Asset", },
      task_type{.uuid_id_ = get_shot_effect_id(),       .name_ = "镜头特效", .short_name_= "镜头特效",   .for_entity_= "Shot", },
      task_type{.uuid_id_ = get_lighting_id(),          .name_ = "灯光",    .short_name_= "灯光",  	  	.for_entity_= "Shot", },
      task_type{.uuid_id_ = get_animation_id(),         .name_ = "动画",    .short_name_= "动画",  	  	.for_entity_= "Shot", },
      task_type{.uuid_id_ = get_simulation_task_id(),   .name_ = "解算任务", .short_name_= "解算任务",   .for_entity_= "Shot", },
  };
  // clang-format on
};

}  // namespace doodle