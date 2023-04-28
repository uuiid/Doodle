//
// Created by td_main on 2023/4/26.
//
#include "doodle_core/core/doodle_lib.h"

#include "boost/lambda2/lambda2.hpp"
#include <boost/lambda2.hpp>

#include "maya_plug/data/cloth_interface.h"
#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/data/qcloth_factory.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/ncloth_factory.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>

#include "cloth_sim.h"
#include "cloth_sim/cloth_sim_factory.h"
#include "entt/entity/fwd.hpp"
#include "range/v3/action/remove_if.hpp"
#include "range/v3/algorithm/for_each.hpp"
#include <exe_maya/data/play_blast.h>
#include <filesystem>
#include <maya/MAnimControl.h>
#include <memory>
namespace doodle::maya_plug {

void cloth_sim::create_ref_file() {
  DOODLE_LOG_INFO("开始扫瞄引用");
  ref_files_ = doodle_lib::Get().ctx().get<reference_file_factory>().create_ref();
}
void cloth_sim::replace_ref_file() {
  DOODLE_LOG_INFO("开始替换引用");
  auto l_j_str = maya_file_io::get_channel_date();
  nlohmann::json l_j{};
  try {
    l_j = nlohmann::json::parse(l_j_str);
  } catch (const nlohmann::json::exception& error) {
    DOODLE_LOG_WARN("解析元数据错误 {}, 取消解析元数据，使用默认元数据", boost::diagnostic_information(error));
  }

  ranges::for_each(ref_files_, [&](entt::handle& in_handle) {
    auto&& l_ref = in_handle.get<reference_file>();
    if (l_j.contains(l_ref.get_key_path())) {
      DOODLE_LOG_INFO("加载元数据 {} l_ref.get_key_path()");
      entt_tool::load_comm<reference_file, sim_cover_attr>(in_handle, l_j.at(l_ref.get_key_path()));
    } else {
      l_ref.use_sim = true;
    }
    if (!l_ref.use_sim) {
      DOODLE_LOG_INFO("引用文件{}不解算", l_ref.get_key_path());
      in_handle.destroy();
    }
  });
  ref_files_ |= ranges::action::remove_if(!boost::lambda2::_1);

  ranges::for_each(ref_files_, [&](entt::handle& in_handle) {
    if (in_handle.get<reference_file>().replace_sim_assets_file()) {
      in_handle.destroy();
    }
  });

  ref_files_ |= ranges::action::remove_if(!boost::lambda2::_1);
}
void cloth_sim::create_cloth() {
  maya_chick(MGlobal::executeCommand(d_str{R"(lockNode -l false -lu false ":initialShadingGroup";)"}));
  DOODLE_LOG_INFO("开始创建布料");
  cloth_factory_interface l_cf{};
  if (qcloth_factory::has_cloth())
    l_cf = std::make_shared<qcloth_factory>();
  else if (ncloth_factory::has_cloth())
    l_cf = std::make_shared<ncloth_factory>();

  if (!l_cf) return;

  cloth_lists_ = l_cf->create_cloth();

  ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
    auto l_c = in_handle.get<cloth_interface>();
    l_c->add_collision({});
    l_c->add_field({});
    l_c->clear_cache();
    l_c->rest();
    /// 指向引用
    l_c->set_cache_folder();
  });
}
void cloth_sim::sim() {
  DOODLE_LOG_INFO("开始解算");
  const MTime k_end_time = MAnimControl::maxTime();
  for (auto&& i = t_post_time_; i < k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
      auto l_c = in_handle.get<cloth_interface>();
      l_c->sim_cloth();
    });
  }
}

void cloth_sim::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p {};

  const MTime k_end_time = MAnimControl::maxTime();
  l_p.conjecture_camera();
  l_p.set_save_dir(maya_file_io::work_path() / "mov");
  l_p.conjecture_ep_sc();
  l_p.play_blast_(anim_begin_time_, k_end_time);
}

void cloth_sim::export_abc() { DOODLE_LOG_INFO("开始导出abc"); }

void cloth_sim::export_fbx() { DOODLE_LOG_INFO("开始导出fbx"); }
}  // namespace doodle::maya_plug
