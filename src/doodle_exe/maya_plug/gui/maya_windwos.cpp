//
// Created by TD on 2021/10/14.
//

#include "maya_windwos.h"

#include <maya/MFileIO.h>
#include <maya_plug/gui/maya_plug_app.h>
namespace doodle::maya_plug {

bool maya_windwos::maya_tool() {
  if (imgui::Button(p_show_name["解析引用"].c_str())) {
    MStringArray file_list;
    auto k_s = MFileIO::getReferences(file_list);
    CHECK_MSTATUS_AND_RETURN(k_s, false);
    std::transform(file_list.begin(),
                   file_list.end(),
                   std::back_inserter(p_file_list),
                   [](const MString& in) -> string {
                     return in.asUTF8();
                   });
  }

  dear::ListBox{p_show_name["引用"].c_str()} && [this]() {
    for (auto& i : p_file_list) {
    
    }
  };

  return false;
}

maya_windwos::maya_windwos()
    : main_windows() {
  p_show_name = make_imgui_name(this,
                                "解析引用",
                                "引用",
                                "设置场景");
}

void maya_windwos::frame_render() {
  main_windows::frame_render();
}
}  // namespace doodle::maya_plug