//
// Created by td_main on 2023/4/26.
//
#include "maya_plug/data/maya_file_io.h"
#include <maya_plug/data/reference_file.h>

#include "cloth_sim.h"
#include <exe_maya/data/play_blast.h>
#include <filesystem>
#include <maya/MAnimControl.h>
namespace doodle::maya_plug {

void cloth_sim::create_ref_file() {
  auto l_list = doodle_lib::Get().ctx().get<reference_file_factory>().create_ref();
  //
  MStatus k_s{};
  auto l_j_str = maya_file_io::get_channel_date();
  std::vector<entt::entity> k_delete{};

  nlohmann::json l_j{};
  try {
    l_j = nlohmann::json::parse(l_j_str);
  } catch (const nlohmann::json::exception& error) {
    DOODLE_LOG_WARN("解析元数据错误 {}, 取消解析元数据，使用默认元数据", boost::diagnostic_information(error));
  }
  for (auto&& i : l_list) {
    auto&& l_ref = i.get<reference_file>();
    if (l_j.contains(l_ref.path)) {
    }
  }
}
void cloth_sim::replace_ref_file() {
  for (auto&& i : ref_files_) {
    i.get<reference_file>().replace_file();
  }
}
void cloth_sim::create_cloth() {}
void cloth_sim::sim() {}

void cloth_sim::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p {};

  MTime k_end_time = MAnimControl::maxTime();
  l_p.conjecture_camera();
  l_p.set_save_dir(maya_file_io::work_path() / "mov");
  l_p.conjecture_ep_sc();
  l_p.play_blast_(anim_begin_time_, k_end_time);
}

void cloth_sim::export_abc() {}

void cloth_sim::export_fbx() {}
}  // namespace doodle::maya_plug
