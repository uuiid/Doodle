//
// Created by td_main on 2023/4/27.
//

#include "export_file_fbx.h"

#include "doodle_core/core/core_help_impl.h"
#include <doodle_core/doodle_core_fwd.h>

#include "maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/reference_file.h>

#include "exception/exception.h"
#include <memory>

namespace doodle::maya_plug {

void export_file_fbx::bake_anim(const MTime& in_start, const MTime& in_end) {
  MStatus k_s{};
  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();
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
  static std::string maya_bakeResults_str{R"(
bakeResults -simulation true -t "{}:{}" -hierarchy below -sampleBy 1 -oversamplingRate 1 -disableImplicitControl true -preserveOutsideKeys {} -sparseAnimCurveBake false -removeBakedAttributeFromLayer false -removeBakedAnimFromLayer false -bakeOnOverrideLayer false -minimizeRotation true -controlPoints false -shape true "{}:*{}";)"};
  auto l_comm =
      fmt::format(maya_bakeResults_str, in_start.value(), in_end.value(), "false"s, m_namespace_, k_cfg.export_group);
  DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
  try {
    k_s = MGlobal::executeCommand(d_str{l_comm});
    DOODLE_MAYA_CHICK(k_s);
  } catch (const std::runtime_error& in) {
    DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用备用参数重试 {}", boost::diagnostic_information(in));
    try {
      l_comm = fmt::format(
          maya_bakeResults_str, in_start.value(), in_end.value(), "true"s, m_namespace_, k_cfg.export_group
      );
      DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
      k_s = MGlobal::executeCommand(d_str{l_comm});
      DOODLE_MAYA_CHICK(k_s);
    } catch (const std::runtime_error& in2) {
      DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用默认参数重试  error {} ", boost::diagnostic_information(in2));

      try {
        l_comm = fmt::format(
            R"(bakeResults  -simulation true -t "{}:{}" -hierarchy below "{}:*{}";)", in_start.value(), in_end.value(),
            m_namespace_, k_cfg.export_group
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

void export_file_fbx::export_anim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view) {
  MSelectionList l_select{};
  MStatus l_satus{};
  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();
  auto& l_ref = in_handle_view.get<reference_file>();
  try {
    l_satus = l_select.add(d_str{fmt::format("{}:{}", l_ref.get_namespace(), k_cfg.export_group)}, true);
    maya_chick(l_satus);
  } catch (const std::runtime_error& err) {
    DOODLE_LOG_WARN("没有物体被配置文件中的 export_group 值选中, 疑似场景文件, 或为不符合配置的文件, 不进行导出");
    return;
  }
  m_namespace_ = l_ref.get_namespace();

  maya_chick(MGlobal::setActiveSelectionList(l_select));
  auto l_arg = in_handle_view.get<generate_file_path_ptr>();
  bake_anim(l_arg->begin_end_time.first, l_arg->begin_end_time.second);

  auto k_file_path = (*l_arg)(l_ref);
  DOODLE_LOG_INFO("导出fbx文件路径 {}", k_file_path);

  auto k_comm = fmt::format("FBXExportBakeComplexStart -v {};", l_arg->begin_end_time.first.value());
  maya_chick(MGlobal::executeCommand(d_str{k_comm}));

  k_comm = fmt::format("FBXExportBakeComplexEnd -v {};", l_arg->begin_end_time.second.value());
  maya_chick(MGlobal::executeCommand(d_str{k_comm}));

  k_comm = std::string{"FBXExportBakeComplexAnimation -v true;"};
  maya_chick(MGlobal::executeCommand(d_str{k_comm}));

  k_comm = std::string{"FBXExportConstraints -v true;"};
  maya_chick(MGlobal::executeCommand(d_str{k_comm}));

  k_comm = fmt::format(R"(FBXExport -f "{}" -s;)", k_file_path.generic_string());
  maya_chick(MGlobal::executeCommand(d_str{k_comm}));
}

void export_file_fbx::export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view) {


  
}

void export_file_fbx::export_cam(const entt::handle_view<generate_file_path_ptr>& in_handle_view) {
  auto& l_arg = in_handle_view.get<generate_file_path_ptr>();
  auto& l_cam = g_reg()->ctx().get<maya_camera>();
  l_cam.unlock_attr();
  l_cam.back_camera(l_arg->begin_end_time.first, l_arg->begin_end_time.second);
  DOODLE_LOG_INFO("开始检查相机是否在世界下方 {}", l_cam.get_transform_name());
  if (l_cam.camera_parent_is_word()) {
    l_cam.fix_group_camera(l_arg->begin_end_time.first, l_arg->begin_end_time.second);
  }
  g_reg()->ctx().get<maya_camera>().export_file(
      l_arg->begin_end_time.first, l_arg->begin_end_time.second,
      *std::dynamic_pointer_cast<reference_file_ns::generate_fbx_file_path>(l_arg)
  );
}

}  // namespace doodle::maya_plug