//
// Created by td_main on 2023/4/27.
//

#include "export_file_fbx.h"

#include <doodle_core/doodle_core_fwd.h>

#include "maya_plug_fwd.h"
#include <maya_plug/abc/alembic_archive_out.h>
#include <maya_plug/data/fbx_write.h>
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sequence_to_blend_shape.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>

#include "data/maya_tool.h"
#include "exception/exception.h"
#include "fmt/core.h"
#include "maya_conv_str.h"
#include <fmt/format.h>
#include <maya/MAnimControl.h>
#include <maya/MApiNamespace.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <memory>
#include <range/v3/view/transform.hpp>
#include <vector>

namespace doodle::maya_plug {
void export_file_fbx::bake_anim(const MTime& in_start, const MTime& in_end, const MDagPath& in_path) {
  MStatus k_s{};
  /**
   *
   * @brief
   * bakeResults(simulation=True,
   *  time=(doodle_work_space.raneg.start,
   *        doodle_work_space.raneg.end),
   *  hierarchy="below",
   *  sampleBy=1,
   *  disableImplicitControl=True,
   *  preserveOutsideKeys=False,
   *  sparseAnimCurveBake=False)
   *
   *  preserveOutsideKeys 这个选项会导致眼睛出现问题
   */
  constexpr static auto maya_bakeResults_str{
      R"(
bakeResults -simulation true -t "{}:{}" -hierarchy below -sampleBy 1 -oversamplingRate 1 -disableImplicitControl true -preserveOutsideKeys {} -sparseAnimCurveBake false -removeBakedAttributeFromLayer false -removeBakedAnimFromLayer false -bakeOnOverrideLayer false -minimizeRotation true -controlPoints false -shape true "{}";)"
  };
  auto l_comm =
      fmt::format(maya_bakeResults_str, in_start.value(), in_end.value(), "false"s, get_node_full_name(in_path));
  DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
  try {
    k_s = MGlobal::executeCommand(d_str{l_comm});
    DOODLE_MAYA_CHICK(k_s);
  } catch (const std::runtime_error& in) {
    DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用备用参数重试 {}", boost::diagnostic_information(in));
    try {
      l_comm =
          fmt::format(maya_bakeResults_str, in_start.value(), in_end.value(), "true"s, get_node_full_name(in_path));
      DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
      k_s = MGlobal::executeCommand(d_str{l_comm});
      DOODLE_MAYA_CHICK(k_s);
    } catch (const std::runtime_error& in2) {
      DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用默认参数重试  error {} ", boost::diagnostic_information(in2));

      try {
        l_comm = fmt::format(
            R"(bakeResults  -simulation true -t "{}:{}" -hierarchy below "{}";)", in_start.value(), in_end.value(),
            get_node_full_name(in_path)
        );
        DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
        k_s = MGlobal::executeCommand(d_str{l_comm});
        DOODLE_MAYA_CHICK(k_s);
      } catch (const std::runtime_error& in3) {
        DOODLE_LOG_INFO("烘培失败, 直接导出 {}", boost::diagnostic_information(in3));
      }
    }

    DOODLE_LOG_INFO("完成烘培, 不检查结果, 直接进行输出");
  }
}

FSys::path export_file_fbx::export_anim(
    const reference_file& in_ref, const generate_file_path_ptr in_gen_file, const MSelectionList& in_exclude
) {
  std::vector<MDagPath> l_export_list{};
  auto l_export_group = in_ref.export_group_attr();
  if (!l_export_group) {
    DOODLE_LOG_WARN("没有物体被配置文件中的 export_group 值选中, 疑似场景文件, 或为不符合配置的文件, 不进行导出");
    return {};
  }

  MItDag l_it{};
  maya_chick(l_it.reset(*l_export_group, MItDag::kDepthFirst, MFn::kMesh));
  MDagPath l_path{};
  for (; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getPath(l_path));
    l_path.pop();
    if (in_exclude.hasItem(l_path)) {
      continue;
    }
    //    if (in_exclude.hasItem(l_path.transform())) {
    //      continue;
    //    }
    l_export_list.push_back(l_path);
  }

  // log_info(fmt::format("导出选中物体 {} 排除物体 {}", l_export_list, in_exclude));

  bake_anim(in_gen_file->begin_end_time.first, in_gen_file->begin_end_time.second, *l_export_group);

  auto l_file_path = (*in_gen_file)(in_ref);
  log_info(fmt::format("导出fbx文件路径 {}", l_file_path));

  fbx_write l_fbx_write{};
  l_fbx_write.set_path(l_file_path);
  l_fbx_write.write(l_export_list, in_gen_file->begin_end_time.first, in_gen_file->begin_end_time.second);
  return l_file_path;
}

FSys::path export_file_fbx::export_rig(const reference_file& in_ref, const std::vector<cloth_interface>& in_cloth) {
  MSelectionList l_select{};
  MDagPath l_main_path{};
  if (auto l_s = l_select.add(d_str{fmt::format(":{}", "UE4")}, true); l_s) {
    l_s = l_select.getDagPath(0, l_main_path);
    maya_chick(l_s);
  } else {
    default_logger_raw()->warn("没有配置中指定的 {} 导出组", "UE4");
    return {};
  }
  std::vector<MDagPath> l_export_list{};

  MItDag l_it{};
  maya_chick(l_it.reset(l_main_path, MItDag::kDepthFirst, MFn::kMesh));
  MDagPath l_path{};
  for (; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getPath(l_path));
    l_path.pop();
    l_export_list.push_back(l_path);
  }

  if (!in_cloth.empty()) {
    std::vector<MDagPath> l_export_sim = in_ref.get_alll_cloth_obj(in_cloth);
    // 排除 export_sim 中的物体
    std::erase_if(l_export_list, [&](const MDagPath& in) {
      return std::ranges::find(l_export_sim, in) != l_export_sim.end();
    });
  }

  default_logger_raw()->info("导出选中物体 {}", fmt::join(l_export_list, "\n"));

  fbx_write l_fbx_write{};
  auto l_file =
      maya_file_io::work_path(FSys::path{"fbx"}) / fmt::format("SK_{}.fbx", maya_file_io::get_current_path().stem());
  if (auto l_p_path = l_file.parent_path(); !FSys::exists(l_p_path)) FSys::create_directories(l_p_path);
  default_logger_raw()->info(fmt::format("导出fbx 文件{}", l_file));
  l_fbx_write.set_path(l_file);
  l_fbx_write.write(l_export_list, MAnimControl::minTime(), MAnimControl::maxTime());
  return l_file;
}

FSys::path export_file_fbx::export_sim(
    const reference_file& in_ref, const generate_file_path_ptr in_gen_file, const std::vector<cloth_interface>& in_cloth
) {
  std::vector<MDagPath> l_export_list{};
  auto l_export_group = in_ref.export_group_attr();
  if (!l_export_group) {
    DOODLE_LOG_WARN("没有物体被配置文件中的 export_group 值选中, 疑似场景文件, 或为不符合配置的文件, 不进行导出");
    return {};
  }

  MItDag l_it{};
  maya_chick(l_it.reset(*l_export_group, MItDag::kDepthFirst, MFn::kMesh));
  MDagPath l_path{};
  for (; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getPath(l_path));
    l_path.pop();
    l_export_list.push_back(l_path);
  }

  // bake_anim(in_gen_file->begin_end_time.first, in_gen_file->begin_end_time.second, *l_export_group);

  auto l_file_path = (*in_gen_file)(in_ref);
  log_info(fmt::format("导出fbx文件路径 {}", l_file_path));

  std::vector<MDagPath> l_export_sim = in_ref.get_alll_cloth_obj(in_cloth);
  // 排除 export_sim 中的物体
  std::erase_if(l_export_list, [&](const MDagPath& in) {
    return std::ranges::find(l_export_sim, in) != l_export_sim.end();
  });

  {
    fbx_write l_fbx_write{};
    l_fbx_write.set_path(l_file_path);
    l_fbx_write.write(l_export_list, in_gen_file->begin_end_time.first, in_gen_file->begin_end_time.second);
  }

  l_file_path.replace_extension(".abc");
  default_logger_raw()->info(fmt::format("导出abc 文件{}", l_file_path));
  {
    alembic::archive_out l_archive_out{
        l_file_path, l_export_sim, in_gen_file->begin_end_time.first, in_gen_file->begin_end_time.second
    };
    for (auto i = in_gen_file->begin_end_time.first; i <= in_gen_file->begin_end_time.second; ++i) {
      MAnimControl::setCurrentTime(i);
      l_archive_out.write();
    }
  }

  return l_file_path;
}

FSys::path export_file_fbx::export_cam(const generate_file_path_ptr& in_gen, std::double_t in_film_aperture) {
  auto l_cam = maya_camera::conjecture();
  l_cam.unlock_attr();
  l_cam.back_camera(in_gen->begin_end_time.first, in_gen->begin_end_time.second);
  auto& l_gen = *std::dynamic_pointer_cast<reference_file_ns::generate_fbx_file_path>(in_gen);
  l_gen.is_camera(true);
  auto l_path = l_gen("");
  {
    fbx_write l_fbx_write{};
    l_fbx_write.set_path(l_path);
    l_fbx_write.write(
        get_dag_path(l_cam.p_path.transform()), in_gen->begin_end_time.first, in_gen->begin_end_time.second,
        in_film_aperture
    );
  }

  return l_path;
}
}  // namespace doodle::maya_plug