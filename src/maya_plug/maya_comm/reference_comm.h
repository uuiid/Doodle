//
// Created by TD on 2021/12/13.
//

#pragma once
#include <main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace {
constexpr char create_ref_file_command_name[] = "doodle_create_ref_file";
constexpr char ref_file_load_command_name[]   = "doodle_ref_file_load";
constexpr char ref_file_sim_command_name[]    = "doodle_ref_file_sim";
constexpr char ref_file_export_command_name[] = "doodle_ref_file_export";
constexpr char load_project_name[]            = "doodle_load_project";
constexpr char set_cloth_cache_path_name[]    = "doodle_set_cloth_cache_path";
}  // namespace
MSyntax ref_file_sim_syntax();
MSyntax ref_file_export_syntax();
MSyntax load_project_syntax();
MSyntax set_cloth_cache_path_syntax();
/**
 * @brief 创建并扫描引用文件句柄
 */
class create_ref_file_command : public TemplateAction<
                                    create_ref_file_command,
                                    create_ref_file_command_name,
                                    null_syntax_t> {
 public:
  MStatus doIt(const MArgList&) override;
};
/**
 * @brief @li 尝试从引用文件的元数据节点中加载以前的保存数据
 * @li 并且替换引用文件
 */
class ref_file_load_command : public TemplateAction<
                                  ref_file_load_command,
                                  ref_file_load_command_name> {
 public:
  MStatus doIt(const MArgList&) override;
};
/**
 * @brief 开始进行解算
 */
class ref_file_sim_command : public TemplateAction<
                                 ref_file_sim_command,
                                 ref_file_sim_command_name,
                                 ref_file_sim_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
/**
 * @brief 导出文件中需要导出的集合体(abc或者fbx)
 */
class ref_file_export_command : public TemplateAction<
                                    ref_file_export_command,
                                    ref_file_export_command_name,
                                    ref_file_export_syntax> {
 public:
  enum export_type : std::uint32_t {
    abc = 0,
    fbx = 1
  };
  MStatus doIt(const MArgList&) override;
};
/**
 * @brief 打开并加载文件
 */
class load_project : public TemplateAction<load_project, load_project_name, load_project_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};

class set_cloth_cache_path : public TemplateAction<
                                 set_cloth_cache_path,
                                 set_cloth_cache_path_name,
                                 set_cloth_cache_path_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
}  // namespace doodle::maya_plug
