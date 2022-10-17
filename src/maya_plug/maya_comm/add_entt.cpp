//
// Created by TD on 2022/10/17.
//

#include "add_entt.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <maya/MArgDatabase.h>
#include <maya/MStatus.h>
#include <nlohmann/json.hpp>
namespace doodle::maya_plug {

namespace add_entt_ns {

constexpr auto json      = "-j";
constexpr auto json_long = "-josn";

MSyntax syntax() {
  MSyntax l_synntax{};
  l_synntax.addFlag(json, json_long, MSyntax::kString);
  return l_synntax;
}
}  // namespace add_entt_ns
MStatus add_entt::doIt(const MArgList& in_arg_list) {
  MStatus l_s{};
  MArgDatabase l_arg_database{syntax(), in_arg_list, &l_s};
  DOODLE_MAYA_CHICK(l_s);

  if (l_arg_database.isFlagSet(add_entt_ns::json, &l_s)) {
    DOODLE_MAYA_CHICK(l_s);

    MString l_str{};
    l_s = l_arg_database.getFlagArgument(add_entt_ns::json, 0, l_str);
    DOODLE_MAYA_CHICK(l_s);

    auto l_json = nlohmann::json::parse(l_str.asUTF8());
    auto l_h    = make_handle();
    entt_tool::load_comm<assets_file, redirection_path_info>(l_h, l_json);
    DOODLE_LOG_INFO("加载实体 {}", l_h);
  }
}
}  // namespace doodle::maya_plug