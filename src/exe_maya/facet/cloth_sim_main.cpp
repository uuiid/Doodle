//
// Created by td_main on 2023/4/26.
//
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/logger/logger.h"

#include "boost/lambda2/lambda2.hpp"
#include <boost/lambda2.hpp>

#include "maya_plug/data/cloth_interface.h"
#include "maya_plug/data/export_file_abc.h"
#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/data/qcloth_factory.h"
#include "maya_plug/maya_plug_fwd.h"
#include <maya_plug/data/export_file_abc.h>
#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/ncloth_factory.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>

#include "cloth_sim.h"
#include "entt/entity/fwd.hpp"
#include "range/v3/action/remove_if.hpp"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/view/transform.hpp"
#include <exe_maya/data/play_blast.h>
#include <filesystem>
#include <map>
#include <maya/MAnimControl.h>
#include <memory>
#include <utility>

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
  ref_files_ |= ranges::actions::remove_if(!boost::lambda2::_1);

  ranges::for_each(ref_files_, [&](entt::handle& in_handle) {
    if (!in_handle.get<reference_file>().replace_sim_assets_file()) {
      in_handle.destroy();
    }
  });

  ref_files_ |= ranges::actions::remove_if(!boost::lambda2::_1);
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
}
void cloth_sim::set_cloth_attr() {
  std::map<std::string, entt::handle> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const entt::handle& in_handle) -> std::pair<std::string, entt::handle> {
                return {in_handle.get<reference_file>().get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
    auto l_c     = in_handle.get<cloth_interface>();
    auto l_ref_h = l_ref_map[l_c->get_namespace()];
    l_c->add_collision(l_ref_h);     /// 添加碰撞
    l_c->rest(l_ref_h);              /// 添加rest
    l_c->cover_cloth_attr(l_ref_h);  /// 添加布料属性
    l_c->add_field(l_ref_h);         /// 添加场力
  });
}
void cloth_sim::sim() {
  DOODLE_LOG_INFO("开始解算");

  std::map<std::string, entt::handle> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const entt::handle& in_handle) -> std::pair<std::string, entt::handle> {
                return {in_handle.get<reference_file>().get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
    auto l_c     = in_handle.get<cloth_interface>();
    auto l_ref_h = l_ref_map[l_c->get_namespace()];
    l_c->set_cache_folder(l_ref_h);  /// 设置缓存文件夹
  });

  /// \brief 在这里我们保存引用
  auto k_save_file = maya_file_io::work_path("ma");
  if (!FSys::exists(k_save_file)) {
    FSys::create_directories(k_save_file);
  }

  k_save_file /= maya_file_io::get_current_path().filename();
  try {
    maya_file_io::save_file(k_save_file);
    DOODLE_LOG_INFO("保存文件到 {}", k_save_file);

  } catch (const maya_error& error) {
    DOODLE_LOG_WARN("无法保存文件 {} : {}", k_save_file, error);
  }
  const MTime k_end_time = MAnimControl::maxTime();
  for (auto&& i = t_post_time_; i < k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    DOODLE_LOG_INFO("解算帧 {}", i);
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

void cloth_sim::export_abc() {
  DOODLE_LOG_INFO("开始导出abc");
  export_file_abc l_ex{};
  auto l_gen             = std::make_shared<reference_file_ns::generate_abc_file_path>();
  const MTime k_end_time = MAnimControl::maxTime();
  l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);

  export_file_fbx l_ex_fbx{};
  ranges::for_each(ref_files_, [&](entt::handle& in_handle) {
    in_handle.emplace<generate_file_path_ptr>(l_gen);
    l_gen->set_fbx_path(false);
    l_ex.export_sim(in_handle);
    l_gen->set_fbx_path(true);
    l_ex_fbx.export_anim(in_handle, l_ex.get_export_list());
  });
}

void cloth_sim::export_fbx() {
  DOODLE_LOG_INFO("开始导出fbx");
  export_file_fbx l_ex{};
  auto l_gen             = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  const MTime k_end_time = MAnimControl::maxTime();
  l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);
  ranges::for_each(ref_files_, [&](entt::handle& in_handle) {
    in_handle.emplace<generate_file_path_ptr>(l_gen);
    l_ex.export_sim(in_handle);
  });
}
}  // namespace doodle::maya_plug
