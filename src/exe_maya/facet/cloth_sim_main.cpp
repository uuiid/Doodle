//
// Created by td_main on 2023/4/26.
//
#include "cloth_sim.h"
#include <exe_maya/data/play_blast.h>
#include <filesystem>
#include <maya/MAnimControl.h>
namespace doodle::maya_plug {

void cloth_sim::create_ref_file() {}
void cloth_sim::replace_ref_file() {}
void cloth_sim::create_cloth() {}
void cloth_sim::sim() {}

void cloth_sim::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p {};

  MTime k_end_time = MAnimControl::maxTime();
  l_p.conjecture_camera();
  //  l_p.set_save_dir(FSys::temp_directory_path() / "doodle" / "maya");
  l_p.play_blast_(anim_begin_time_, k_end_time);
}

void cloth_sim::export_abc() {}

void cloth_sim::export_fbx() {}
}  // namespace doodle::maya_plug
