//
// Created by TD on 2021/6/24.
//

#include "setting_windows.h"

#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/core/CoreSet.h>

#include <magic_enum.hpp>
namespace doodle {

setting_windows::setting_windows(nana::window in_window)
    : nana::form(in_window, nana::API::make_center(in_window, 600, 260)),
      p_layout(*this),
      p_dep_label(*this, "部门： "),
      p_user_label(*this, "用户： "),
      p_cache_label(*this, "缓存： "),
      p_doc_label(*this, "文档： "),
      p_maya_label(*this, "maya路径： "),
      p_ue_path_label(*this, "ue路径： "),
      p_ue_version_label(*this, "ue版本： "),
      p_ue_shot_start_label(*this, "ue镜头默认开始： "),
      p_ue_shot_end_label(*this, "ue镜头默认结束： "),
      p_dep(*this),
      p_user(*this),
      p_cache(*this),
      p_doc(*this),
      p_maya(*this),
      p_ue_path(*this),
      p_ue_version(*this),
      p_ue_shot_start(*this),
      p_ue_shot_end(*this) {
  p_layout.div(
      R"(<vertical
                  <dep>
                  <user>
                  <cache>
                  <doc>
                  <maya>
                  <ue_path>
                  <ue_version>
                  <ue_shot_start>
                  <ue_shot_end>
                   >)");
  p_layout.field("dep")
      << p_dep_label
      << p_dep;
  p_layout.field("user")
      << p_user_label
      << p_user;
  p_layout.field("cache")
      << p_cache_label
      << p_cache;
  p_layout.field("doc")
      << p_doc_label
      << p_doc;
  p_layout.field("maya")
      << p_maya_label
      << p_maya;
  p_layout.field("ue_path")
      << p_ue_path_label
      << p_ue_path;
  p_layout.field("ue_version")
      << p_ue_version_label
      << p_ue_version;
  p_layout.field("ue_shot_start")
      << p_ue_shot_start_label
      << p_ue_shot_start;
  p_layout.field("ue_shot_end")
      << p_ue_shot_end_label
      << p_ue_shot_end;

  p_dep.editable(false);
  p_user.multi_lines(false);
  p_maya.multi_lines(false);
  p_ue_path.multi_lines(false);
  p_ue_version.multi_lines(false);
  p_ue_shot_start.range(0, 999, 1);
  p_ue_shot_end.range(0, 999, 1);
  for (auto& str : magic_enum::enum_names<Department>()) {
    p_dep.push_back(std::string{str});
  }
  auto& set = CoreSet::getSet();
  p_dep.events().selected.connect([&set](const nana::arg_combox& in_) {
    set.setDepartment(in_.widget.caption());
  });
  p_user.events().text_changed.connect([&set](const nana::arg_textbox& in_) {
    set.setUser(in_.widget.text());
  });
  p_maya.events().text_changed.connect([&set](const nana::arg_textbox& in_) {
    set.setMayaPath(in_.widget.text());
  });
  p_ue_path.events().text_changed.connect([&set](const nana::arg_textbox& in_) {
    set.gettUe4Setting().setPath(in_.widget.text());
  });
  p_ue_version.events().text_changed.connect([&set](const nana::arg_textbox& in_) {
    set.gettUe4Setting().setVersion(in_.widget.text());
  });
  p_ue_shot_start.events().text_changed.connect([&set](const nana::arg_spinbox& in_) {
    set.gettUe4Setting().setShotStart(in_.widget.to_int());
  });
  p_ue_shot_end.events().text_changed.connect([&set](const nana::arg_spinbox& in_) {
    set.gettUe4Setting().setShotEnd(in_.widget.to_int());
  });
  init_setting();
  p_layout.collocate();
  events().destroy([&set, this](const nana::arg_destroy& in_) {
    try {
      set.writeDoodleLocalSet();
    } catch (const std::runtime_error& error) {
      DOODLE_LOG_WARN(error.what());
      nana::msgbox mes{*this, error.what()};
      mes();
    }
  });
}
void setting_windows::init_setting() {
  auto& set = CoreSet::getSet();
  p_dep.caption(set.getDepartment());
  p_user.reset(set.getUser());
  p_cache.caption(set.getCacheRoot().generic_string());
  p_doc.caption(set.getDoc().generic_string());
  p_maya.reset(set.MayaPath().generic_string());
  p_ue_path.reset(set.gettUe4Setting().Path().generic_string());
  p_ue_version.reset(set.gettUe4Setting().Version());
  p_ue_shot_start.value(std::to_string(set.gettUe4Setting().ShotStart()));
  p_ue_shot_end.value(std::to_string(set.gettUe4Setting().ShotEnd()));
}
}  // namespace doodle
