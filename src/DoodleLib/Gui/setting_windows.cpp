//
// Created by TD on 2021/6/24.
//

#include "setting_windows.h"

#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Project.h>
#include <core/CoreSet.h>

#include <magic_enum.hpp>
namespace doodle {

setting_windows::setting_windows(nana::window in_window)
    : nana::form(in_window, nana::API::make_center(300, 400)),
      p_layout(*this),
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
      << nana::label{*this, "部门： "}
      << p_dep;
  p_layout.field("user")
      << nana::label{*this, "用户： "}
      << p_user;
  p_layout.field("cache")
      << nana::label{*this, "缓存： "}
      << p_cache;
  p_layout.field("doc")
      << nana::label{*this, "文档： "}
      << p_doc;
  p_layout.field("maya")
      << nana::label{*this, "maya路径： "}
      << p_maya;
  p_layout.field("ue_path")
      << nana::label{*this, "ue路径： "}
      << p_ue_path;
  p_layout.field("ue_version")
      << nana::label{*this, "ue版本： "}
      << p_ue_version;
  p_layout.field("ue_shot_start")
      << nana::label{*this, "ue镜头默认开始： "}
      << p_ue_shot_start;
  p_layout.field("ue_shot_end")
      << nana::label{*this, "ue镜头默认结束： "}
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
}
void setting_windows::init_setting() {
  auto& set = CoreSet::getSet();
  p_dep.caption(set.getDepartment());
  p_user.reset(set.getUser());
  p_cache.caption(set.getCacheRoot());
  p_doc.caption(set.getDoc());
  p_maya.reset(set.MayaPath().generic_string());
  p_ue_path.reset(set.gettUe4Setting().Path().generic_string());
  p_ue_version.reset(set.gettUe4Setting().Version());
  p_ue_shot_start.value(std::to_string(set.gettUe4Setting().ShotStart()));
  p_ue_shot_end.value(std::to_string(set.gettUe4Setting().ShotEnd()));
}
}  // namespace doodle
