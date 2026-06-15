//
// Created by TD on 25-7-17.
//
#include "entity_type.h"

#include <doodle_core/metadata/assets.h>
namespace doodle {

details::assets_type_enum convert_assets_type_enum(const uuid& in_assets_type_id) {
  if (in_assets_type_id == asset_type::get_character_id()) {
    return details::assets_type_enum::character;
  }
  if (in_assets_type_id == asset_type::get_ground_id()) {
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