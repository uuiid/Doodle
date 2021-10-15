//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

#include <nlohmann/json.hpp>

namespace doodle::maya_plug {

namespace reference_attr {

bool data::operator==(const data& in_rhs) const {
  return path == in_rhs.path;
}
bool data::operator!=(const data& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace reference_attr

reference_attr_setting::reference_attr_setting()
    : command_base(),
      p_list() {
  p_name     = "引用编辑";
  p_show_str = make_imgui_name(this,
                               "解析引用",
                               "引用",
                               "设置场景",
                               "保存");
}

bool reference_attr_setting::get_file_info() {
  MStringArray file_list;
  MStatus k_status{};
  k_status = MFileIO::getReferences(file_list);
  CHECK_MSTATUS_AND_RETURN(k_status, false);
  p_list.clear();
  std::transform(file_list.begin(),
                 file_list.end(),
                 std::back_inserter(p_list),
                 [](const MString& in) -> reference_attr::data_ptr {
                   auto k_r     = new_object<reference_attr::data>();
                   k_r->path    = in.asUTF8();
                   k_r->use_sim = false;
                   return k_r;
                 });

  MString k_result{};
  MGlobal::executeCommand(R"(fileInfo -q "doodle_sim_json")", k_result, &k_status);
  CHECK_MSTATUS_AND_RETURN(k_status, false);
  if (k_result.numChars() != 0) {
    auto k_j = nlohmann::json::parse(k_result.asUTF8());

    for (auto& k_i : k_j) {
      auto k_d = k_i.get<reference_attr::data>();
      for (auto& j : p_list) {
        if (*j == k_d)
          j->use_sim = k_d.use_sim;
      }
    }
  }
  return true;
}

bool reference_attr_setting::render() {
  if (imgui::Button(p_show_str["解析引用"].c_str())) {
    get_file_info();
  }

  dear::ListBox{p_show_str["引用"].c_str()} && [this]() {
    for (auto& i : p_list) {
      imgui::Checkbox(i->path.c_str(), &(i->use_sim));
    }
  };
  if (imgui::Button(p_show_str["保存"].c_str())) {
    std::vector<reference_attr::data> k_l;
    std::transform(p_list.begin(), p_list.end(), std::back_inserter(k_l), [](auto& i) { return *i; });
    nlohmann::json k_j{k_l};
    auto str = fmt::format(R"(fileInfo "doodle_sim_json" "{}")", k_j.dump());
    MString k_ms;
    k_ms.setUTF8(str.c_str());
    MGlobal::executeCommand(k_ms);
  }
  return true;
}

}  // namespace doodle::maya_plug
