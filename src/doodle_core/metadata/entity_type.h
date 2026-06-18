//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct DOODLE_CORE_API task_type_asset_type_link {
  std::int64_t id_;
  uuid asset_type_id_;
  uuid task_type_id_;
};
struct DOODLE_CORE_API asset_type {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string short_name_;
  std::string description_;
  std::vector<uuid> task_types_;
  bool archived_;

  constexpr static uuid get_shot_id() {
    // c8b65ac0-e0b7-4da5-b4d2-9b466be0788e
    return {{0xc8, 0xb6, 0x5a, 0xc0, 0xe0, 0xb7, 0x4d, 0xa5, 0xb4, 0xd2, 0x9b, 0x46, 0x6b, 0xe0, 0x78, 0x8e}};
  }
  constexpr static uuid get_episode_id() {
    // 8a5067a7-f1d8-4954-9a1e-ac70738fa00d
    return {{0x8a, 0x50, 0x67, 0xa7, 0xf1, 0xd8, 0x49, 0x54, 0x9a, 0x1e, 0xac, 0x70, 0x73, 0x8f, 0xa0, 0x0d}};
  }
  constexpr static uuid get_sequence_id() {
    // e00f8293-81d8-4f04-8309-a6c2e0f652ec
    return {{0xe0, 0x0f, 0x82, 0x93, 0x81, 0xd8, 0x4f, 0x04, 0x83, 0x09, 0xa6, 0xc2, 0xe0, 0xf6, 0x52, 0xec}};
  }
  constexpr static uuid get_concept_id() {
    // ed6e260b-dc5b-4702-8060-6ecd099f1e33
    return {{0xed, 0x6e, 0x26, 0x0b, 0xdc, 0x5b, 0x47, 0x02, 0x80, 0x60, 0x6e, 0xcd, 0x09, 0x9f, 0x1e, 0x33}};
  }
  constexpr static uuid get_edit_id() {
    // 91a1a772-668d-4880-b0d6-43d476fe9ac2
    return {{0x91, 0xa1, 0xa7, 0x72, 0x66, 0x8d, 0x48, 0x80, 0xb0, 0xd6, 0x43, 0xd4, 0x76, 0xfe, 0x9a, 0xc2}};
  }
  constexpr static uuid get_scene_id() {
    // 9d746cfa-445a-4091-a774-d83b14acab02
    return {{0x9d, 0x74, 0x6c, 0xfa, 0x44, 0x5a, 0x40, 0x91, 0xa7, 0x74, 0xd8, 0x3b, 0x14, 0xac, 0xab, 0x02}};
  }

  // 角色
  constexpr static uuid get_character_id() {
    // f9a8be37-2d05-4e20-8fae-751a61960ce4
    return {{0xf9, 0xa8, 0xbe, 0x37, 0x2d, 0x05, 0x4e, 0x20, 0x8f, 0xae, 0x75, 0x1a, 0x61, 0x96, 0x0c, 0xe4}};
  }
  // 场景
  constexpr static uuid get_ground_id() {
    // 0e40cd9b-7f50-418b-8322-39c451f49dde
    return {{0x0e, 0x40, 0xcd, 0x9b, 0x7f, 0x50, 0x41, 0x8b, 0x83, 0x22, 0x39, 0xc4, 0x51, 0xf4, 0x9d, 0xde}};
  }
  // 场景资产
  constexpr static uuid get_scene_asset_id() {
    // 2e869265-f7d6-436e-83aa-516eb7d68eae
    return {{0x2e, 0x86, 0x92, 0x65, 0xf7, 0xd6, 0x43, 0x6e, 0x83, 0xaa, 0x51, 0x6e, 0xb7, 0xd6, 0x8e, 0xae}};
  }

  // 道具
  constexpr static uuid get_prop_id() {
    // 8c02b76a-6be6-4959-af58-5c31a85fe072
    return {{0x8c, 0x02, 0xb7, 0x6a, 0x6b, 0xe6, 0x49, 0x59, 0xaf, 0x58, 0x5c, 0x31, 0xa8, 0x5f, 0xe0, 0x72}};
  }
  // 特效
  constexpr static uuid get_effect_id() {
    // 6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3
    return {{0x6d, 0x9d, 0x69, 0xf0, 0x42, 0x69, 0x46, 0xfc, 0x9c, 0x26, 0xa7, 0xf7, 0xbf, 0x2f, 0x30, 0xe3}};
  }
  // AI资产
  constexpr static uuid get_ai_id() {
    // 6461808e-5fc6-4182-b99a-44b7855249ed
    return {{0x64, 0x61, 0x80, 0x8e, 0x5f, 0xc6, 0x41, 0x82, 0xb9, 0x9a, 0x44, 0xb7, 0x85, 0x52, 0x49, 0xed}};
  }
  // 半AI资产
  constexpr static uuid get_half_ai_id() {
    // 9c966068-cde6-433e-9a2c-b7235739716b
    return {{0x9c, 0x96, 0x60, 0x68, 0xcd, 0xe6, 0x43, 0x3e, 0x9a, 0x2c, 0xb7, 0x23, 0x57, 0x39, 0x71, 0x6b}};
  }
  constexpr static std::array<asset_type, 13> get_all_constant();

  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, asset_type& p) {
    j.at("name").get_to(p.name_);
    j.at("short_name").get_to(p.short_name_);
    if (j.contains("description") && j.at("description").is_string()) j.at("description").get_to(p.description_);
    if (j.contains("task_types")) j.at("task_types").get_to(p.task_types_);
    j.at("archived").get_to(p.archived_);
  }
  // to json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const asset_type& p) {
    j["id"]         = p.uuid_id_;
    j["name"]       = p.name_;
    j["short_name"] = p.short_name_;
    if (p.description_.empty())
      j["description"] = nlohmann::json::value_t::null;
    else
      j["description"] = p.description_;
    j["task_types"] = p.task_types_;
    j["archived"]   = p.archived_;
  }
};

constexpr std::array<asset_type, 13> asset_type::get_all_constant() {
  // clang-format off
  return {
      asset_type{.uuid_id_ = get_shot_id(),     .name_ = "Shot",     .short_name_ = "Shot", },
      asset_type{.uuid_id_ = get_episode_id(),  .name_ = "Episode",  .short_name_ = "Episode", },
      asset_type{.uuid_id_ = get_sequence_id(), .name_ = "Sequence", .short_name_ = "Sequence", },
      asset_type{.uuid_id_ = get_concept_id(),  .name_ = "Concept",  .short_name_ = "Concept", },
      asset_type{.uuid_id_ = get_edit_id(),     .name_ = "Edit",     .short_name_ = "Edit", },
      asset_type{.uuid_id_ = get_scene_id(),    .name_ = "Scene",    .short_name_ = "Scene", },

      asset_type{.uuid_id_ = get_character_id(), .name_ = "角色", .short_name_ = "角色", },
      asset_type{.uuid_id_ = get_ground_id(),    .name_ = "场景", .short_name_ = "场景", },
      asset_type{.uuid_id_ = get_prop_id(),      .name_ = "道具", .short_name_ = "道具", },
      asset_type{.uuid_id_ = get_effect_id(),    .name_ = "特效", .short_name_ = "特效", },
      asset_type{.uuid_id_ = get_ai_id(),        .name_ = "AI",   .short_name_ = "AI", },
      asset_type{.uuid_id_ = get_half_ai_id(),   .name_ = "半AI", .short_name_ = "半AI", },
      asset_type{.uuid_id_ = get_scene_asset_id(), .name_ = "场景资产", .short_name_ = "场景资产", },
  };
  // clang-format on
}

}  // namespace doodle
