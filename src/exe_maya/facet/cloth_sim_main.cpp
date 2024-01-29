//
// Created by td_main on 2023/4/26.
//
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/logger/logger.h"

#include <doodle_lib/exe_warp/maya_exe.h>

#include "boost/lambda2/lambda2.hpp"
#include <boost/lambda2.hpp>

#include "maya_plug/data/cloth_interface.h"
#include "maya_plug/data/export_file_abc.h"
#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/data/qcloth_factory.h"
#include "maya_plug/maya_plug_fwd.h"
#include <maya_plug/data/export_file_abc.h>
#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/maya_camera.h>
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
  all_ref_files_ = g_ctx().get<reference_file_factory>().create_ref();
}
void cloth_sim::replace_ref_file() {
  DOODLE_LOG_INFO("开始替换引用");
  ref_files_ = all_ref_files_ | ranges::views::filter([](const entt::handle& in_handle) -> bool {
                 auto&& l_ref = in_handle.get<reference_file>();
                 if (l_ref.export_group_attr() && l_ref.get_use_sim() && l_ref.has_sim_assets_file()) {
                   return l_ref.replace_sim_assets_file();
                 } else {
                   default_logger_raw()->log(log_loc(), level::info, "引用文件{}不解算", l_ref.get_abs_path());
                 }
                 return false;
               }) |
               ranges::to<decltype(ref_files_)>;
}
void cloth_sim::create_cloth() {
  DOODLE_LOG_INFO("开始解锁节点 initialShadingGroup");
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
    l_c->set_cache_folder(l_ref_h, true);  /// 设置缓存文件夹
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
  for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    DOODLE_LOG_INFO("解算帧 {}", i);
    ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
      auto l_c = in_handle.get<cloth_interface>();
      l_c->sim_cloth();
    });
  }
}

void cloth_sim::touch_sim() {
  DOODLE_LOG_INFO("开始触摸解算");

  std::map<std::string, entt::handle> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const entt::handle& in_handle) -> std::pair<std::string, entt::handle> {
                return {in_handle.get<reference_file>().get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](entt::handle& in_handle) {
    auto l_c     = in_handle.get<cloth_interface>();
    auto l_ref_h = l_ref_map[l_c->get_namespace()];
    l_c->set_cache_folder(l_ref_h, false);  /// 设置缓存文件夹
  });

  const MTime k_end_time = MAnimControl::maxTime();
  for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    DOODLE_LOG_INFO("测试解算帧 {}", i);
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
    auto l_sim_path = l_ex.export_sim(in_handle);
    for (auto&& i : l_sim_path) {
      if (!i.empty()) out_and_ref_file_list_.emplace_back(i, in_handle.get<reference_file>().get_abs_path());
    }
    if (l_ex.get_export_list().isEmpty()) return;
    l_gen->set_fbx_path(true);
    auto l_path = l_ex_fbx.export_anim(in_handle, l_ex.get_export_list());
    if (!l_path.empty()) {
      out_and_ref_file_list_.emplace_back(l_path, in_handle.get<reference_file>().get_abs_path());
    }
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
void cloth_sim::export_anim_file() {
  DOODLE_LOG_INFO("开始导出动画文件");
  export_file_fbx l_ex{};
  auto l_gen             = std::make_shared<reference_file_ns::generate_fbx_file_path>();
  const MTime k_end_time = MAnimControl::maxTime();
  l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);
  ranges::for_each(
      all_ref_files_ | ranges::views::filter([&](const entt::handle& in_handle) -> bool {
        return ranges::find(ref_files_, in_handle) != ref_files_.end();
      }),
      [&](entt::handle& in_handle) {
        auto& l_ref = in_handle.get<reference_file>();
        if (!l_ref.is_loaded()) l_ref.load_file();
        in_handle.emplace<generate_file_path_ptr>(l_gen);
        auto l_path = l_ex.export_anim(in_handle);
        if (!l_path.empty()) {
          out_and_ref_file_list_.emplace_back(l_path, in_handle.get<reference_file>().get_abs_path());
        }
      }
  );
  // 导出相机
  g_reg()->ctx().emplace<maya_camera>().conjecture();
  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace<generate_file_path_ptr>(l_gen);
  auto l_cam_path = l_ex.export_cam(l_h);
  out_and_ref_file_list_.emplace_back(l_cam_path, FSys::path{});
}
void cloth_sim::write_config() {
  default_logger_raw()->log(log_loc(), level::info, "导出动画文件完成, 开始写出配置文件");

  maya_exe_ns::maya_out_arg l_out_arg{};
  l_out_arg.begin_time = anim_begin_time_.value();
  l_out_arg.end_time   = MAnimControl::maxTime().value();
  for (auto&& i : out_and_ref_file_list_) {
    l_out_arg.out_file_list.emplace_back(i.first, i.second);
  }
  nlohmann::json l_json = l_out_arg;
  if (!out_path_file_.empty()) {
    if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
    default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
    FSys::ofstream{out_path_file_} << l_json.dump(4);
  } else
    log_info(fmt::format("导出文件 {}", l_json.dump(4)));
}

}  // namespace doodle::maya_plug
