//
// Created by td_main on 2023/10/18.
//

#pragma once
#include <nlohmann/json.hpp>
#include <string>
namespace doodle::render_farm::detail {
struct ue4_arg {
 public:
  std::string ProjectPath;
  std::string Args;
  std::string ManifestValue;

  std::string out_file_path;

  // to json
  friend void to_json(nlohmann::json& j, const ue4_arg& in_arg) {
    j["projectPath"]   = in_arg.ProjectPath;
    j["args"]          = in_arg.Args;
    j["manifestValue"] = in_arg.ManifestValue;
    j["outFilePath"]   = in_arg.out_file_path;
  }

  // form json
  friend void from_json(const nlohmann::json& j, ue4_arg& in_arg) {
    j.at("projectPath").get_to(in_arg.ProjectPath);
    j.at("args").get_to(in_arg.Args);
    j.at("manifestValue").get_to(in_arg.ManifestValue);
    j.at("outFilePath").get_to(in_arg.out_file_path);
  }
};
}  // namespace doodle::render_farm::detail
