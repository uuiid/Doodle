//
// Created by td_main on 2023/4/26.
//
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/logger/logger.h"

#include <doodle_lib/exe_warp/maya_exe.h>

#include "boost/lambda2/lambda2.hpp"
#include <boost/lambda2.hpp>

#include "maya_plug/data/cloth_interface.h"
#include "maya_plug/data/maya_file_io.h"
#include "maya_plug/data/qcloth_factory.h"
#include "maya_plug/maya_plug_fwd.h"
#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/ncloth_factory.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/fmt/fmt_dag_path.h>

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
  ref_files_ =
      all_ref_files_ | ranges::views::filter([this](const reference_file& in_handle) -> bool {
        if (in_handle.export_group_attr() && in_handle.get_use_sim() && in_handle.has_sim_assets_file(sim_file_map_)) {
          return true;
        } else {
          default_logger_raw()->log(log_loc(), level::info, "引用文件{}不解算", in_handle.get_abs_path());
        }
        return false;
      }) |
      ranges::to<decltype(ref_files_)>;
}
void cloth_sim::replace_ref_file() {
  DOODLE_LOG_INFO("开始替换引用");
  ref_files_ = ref_files_ | ranges::views::filter([this](reference_file& in_handle) -> bool {
                 return in_handle.replace_sim_assets_file(sim_file_map_);
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
  std::map<std::string, reference_file> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const reference_file& in_handle) -> std::pair<std::string, reference_file> {
                return {in_handle.get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  cloth_lists_ |= ranges::actions::remove_if([&](const cloth_interface& in_handle) -> bool {
    if (l_ref_map.contains(in_handle->get_namespace())) {
      return false;
    }
    default_logger_raw()->log(
        log_loc(), level::info, "布料{}未找到对应的引用文件, 无法导出, 不进行解算, 请查找对应的引用",
        in_handle->get_shape()
    );
    return true;
  });
}
void cloth_sim::set_cloth_attr() {
  std::map<std::string, reference_file> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const reference_file& in_handle) -> std::pair<std::string, reference_file> {
                return {in_handle.get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) {
    auto l_ref_h = l_ref_map[in_handle->get_namespace()];
    in_handle->add_collision(l_ref_h);     /// 添加碰撞
    in_handle->rest();                     /// 添加rest
    in_handle->cover_cloth_attr(l_ref_h);  /// 添加布料属性
    in_handle->add_field(l_ref_h);         /// 添加场力
  });
}
void cloth_sim::sim() {
  DOODLE_LOG_INFO("开始解算");

  std::map<std::string, reference_file> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const reference_file& in_handle) -> std::pair<std::string, reference_file> {
                return {in_handle.get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) {
    auto l_ref_h = l_ref_map[in_handle->get_namespace()];
    in_handle->set_cache_folder(l_ref_h, true);  /// 设置缓存文件夹
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
  } catch (const std::runtime_error& error) {
    DOODLE_LOG_WARN("无法保存文件 {} : {}", k_save_file, error.what());
  }
  const MTime k_end_time = MAnimControl::maxTime();
  for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    DOODLE_LOG_INFO("解算帧 {}", i);
    ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) { in_handle->sim_cloth(); });
  }
}

void cloth_sim::touch_sim() {
  DOODLE_LOG_INFO("开始触摸解算");

  std::map<std::string, reference_file> l_ref_map{};
  l_ref_map = ref_files_ |
              ranges::views::transform([](const reference_file& in_handle) -> std::pair<std::string, reference_file> {
                return {in_handle.get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;

  ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) {
    auto l_ref_h = l_ref_map[in_handle->get_namespace()];
    in_handle->set_cache_folder_read_only();  // 设置缓存为只读
  });

  const MTime k_end_time = MAnimControl::maxTime();
  for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
    maya_chick(MAnimControl::setCurrentTime(i));
    DOODLE_LOG_INFO("测试解算帧 {}", i);
    ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) { in_handle->sim_cloth(); });
  }
}

void cloth_sim::play_blast() {
  DOODLE_LOG_INFO("开始排屏");
  class play_blast l_p{};

  const MTime k_end_time  = MAnimControl::maxTime();

  out_arg_.movie_file_dir = l_p.play_blast_(anim_begin_time_, k_end_time, size_);
}

void cloth_sim::export_abc() {
  DOODLE_LOG_INFO("开始导出解算fbx");
  auto l_gen             = std::make_shared<reference_file_ns::generate_abc_file_path>();
  const MTime k_end_time = MAnimControl::maxTime();
  l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);

  export_file_fbx l_ex_fbx{};
  ranges::for_each(ref_files_, [&](reference_file& in_handle) {
    l_gen->set_fbx_path(true);
    auto l_path = l_ex_fbx.export_sim(in_handle, l_gen);
    for (auto& p : l_path) out_arg_.out_file_list.emplace_back(p);
  });
}

void cloth_sim::export_anim_file() {
  DOODLE_LOG_INFO("开始导出动画文件");
  export_file_fbx l_ex{};
  auto l_gen             = std::make_shared<reference_file_ns::generate_abc_file_path>();
  const MTime k_end_time = MAnimControl::maxTime();
  l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);
  l_gen->set_fbx_path(true);
  ranges::for_each(
      all_ref_files_ | ranges::views::filter([&](const reference_file& in_handle) -> bool {
        return ranges::find(ref_files_, in_handle) == ref_files_.end();
      }),
      [&](reference_file& in_handle) {
        if (!in_handle.is_loaded()) in_handle.load_file();
        auto l_path = l_ex.export_anim(in_handle, l_gen);
        out_arg_.out_file_list.emplace_back(l_path);
      }
  );

  // 导出相机
  camera_path_ = l_ex.export_cam(l_gen, film_aperture_);
  out_arg_.out_file_list.emplace_back(camera_path_);  // 导出相机
}
void cloth_sim::write_config() {
  default_logger_raw()->log(log_loc(), level::info, "导出动画文件完成, 开始写出配置文件");
  out_arg_.begin_time   = anim_begin_time_.value();
  out_arg_.end_time     = MAnimControl::maxTime().value();

  nlohmann::json l_json = out_arg_;
  if (!out_path_file_.empty()) {
    if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
    default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
    FSys::ofstream{out_path_file_} << l_json.dump(4);
  } else
    log_info(fmt::format("导出文件 {}", l_json.dump(4)));
}

}  // namespace doodle::maya_plug
