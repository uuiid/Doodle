//
// Created by TD on 25-7-17.
//
#include "entity_type.h"

#include <doodle_core/metadata/assets.h>
namespace doodle {

uuid asset_type::get_concept_id() {
  // ed6e260b-dc5b-4702-8060-6ecd099f1e33
  const uuid l_id{{0xed, 0x6e, 0x26, 0x0b, 0xdc, 0x5b, 0x47, 0x02, 0x80, 0x60, 0x6e, 0xcd, 0x09, 0x9f, 0x1e, 0x33}};
  return l_id;
}
uuid asset_type::get_edit_id() {
  // 91a1a772-668d-4880-b0d6-43d476fe9ac2
  const uuid l_id{{0x91, 0xa1, 0xa7, 0x72, 0x66, 0x8d, 0x48, 0x80, 0xb0, 0xd6, 0x43, 0xd4, 0x76, 0xfe, 0x9a, 0xc2}};
  return l_id;
}
uuid asset_type::get_episode_id() {
  // 8a5067a7-f1d8-4954-9a1e-ac70738fa00d
  const uuid l_id{{0x8a, 0x50, 0x67, 0xa7, 0xf1, 0xd8, 0x49, 0x54, 0x9a, 0x1e, 0xac, 0x70, 0x73, 0x8f, 0xa0, 0x0d}};
  return l_id;
}
uuid asset_type::get_scene_id() {
  // 9d746cfa-445a-4091-a774-d83b14acab02
  const uuid l_id{{0x9d, 0x74, 0x6c, 0xfa, 0x44, 0x5a, 0x40, 0x91, 0xa7, 0x74, 0xd8, 0x3b, 0x14, 0xac, 0xab, 0x02}};
  return l_id;
}
uuid asset_type::get_sequence_id() {
  // e00f8293-81d8-4f04-8309-a6c2e0f652ec
  const uuid l_id{{0xe0, 0x0f, 0x82, 0x93, 0x81, 0xd8, 0x4f, 0x04, 0x83, 0x09, 0xa6, 0xc2, 0xe0, 0xf6, 0x52, 0xec}};
  return l_id;
}
uuid asset_type::get_shot_id() {
  // c8b65ac0-e0b7-4da5-b4d2-9b466be0788e
  const uuid l_id{{0xc8, 0xb6, 0x5a, 0xc0, 0xe0, 0xb7, 0x4d, 0xa5, 0xb4, 0xd2, 0x9b, 0x46, 0x6b, 0xe0, 0x78, 0x8e}};
  return l_id;
}

uuid asset_type::get_character_id() {
  // f9a8be37-2d05-4e20-8fae-751a61960ce4
  const uuid l_id{{0xf9, 0xa8, 0xbe, 0x37, 0x2d, 0x05, 0x4e, 0x20, 0x8f, 0xae, 0x75, 0x1a, 0x61, 0x96, 0x0c, 0xe4}};
  return l_id;
}

uuid asset_type::get_effect_id() {
  // 6d9d69f0-4269-46fc-9c26-a7f7bf2f30e3
  const uuid l_id{{0x6d, 0x9d, 0x69, 0xf0, 0x42, 0x69, 0x46, 0xfc, 0x9c, 0x26, 0xa7, 0xf7, 0xbf, 0x2f, 0x30, 0xe3}};
  return l_id;
}

uuid asset_type::get_ground_id() {
  // 0e40cd9b-7f50-418b-8322-39c451f49dde
  const uuid l_id{{0x0e, 0x40, 0xcd, 0x9b, 0x7f, 0x50, 0x41, 0x8b, 0x83, 0x22, 0x39, 0xc4, 0x51, 0xf4, 0x9d, 0xde}};
  return l_id;
}

uuid asset_type::get_prop_id() {
  // 8c02b76a-6be6-4959-af58-5c31a85fe072
  const uuid l_id{{0x8c, 0x02, 0xb7, 0x6a, 0x6b, 0xe6, 0x49, 0x59, 0xaf, 0x58, 0x5c, 0x31, 0xa8, 0x5f, 0xe0, 0x72}};
  return l_id;
}
details::assets_type_enum convert_assets_type_enum(const uuid& in_assets_type_id) {
  if (in_assets_type_id == asset_type::get_character_id()) {
    return details::assets_type_enum::character;
  }
  if (in_assets_type_id == asset_type::get_scene_id()) {
    return details::assets_type_enum::scene;
  }
  // if (in_assets_type_id == asset_type::get_ground_id()) {
  // return details::assets_type_enum::ground;
  // }
  if (in_assets_type_id == asset_type::get_prop_id()) {
    return details::assets_type_enum::prop;
  }
  if (in_assets_type_id == asset_type::get_effect_id()) {
    return details::assets_type_enum::vfx;
  }
  // if (in_assets_type_id == asset_type::get_shot_id()) {
  //   return details::assets_type_enum::shot;
  // }
  // if (in_assets_type_id == asset_type::get_sequence_id()) {
  //   return details::assets_type_enum::sequence;
  // }
  // if (in_assets_type_id == asset_type::get_episode_id()) {
  //   return details::assets_type_enum::episode;
  // }
  throw_exception(doodle_error{"未知的资产类型ID: {}", in_assets_type_id});
}

}  // namespace doodle