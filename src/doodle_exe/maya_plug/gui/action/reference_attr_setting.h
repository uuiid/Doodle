//
// Created by TD on 2021/10/14.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <maya/MSelectionList.h>
#include <nlohmann/json.hpp>
#include <maya/MTemplateCommand.h>

namespace doodle::maya_plug {
class reference_file;

namespace reference_attr {
class data {
 public:
  string path;
  bool use_sim;
  bool operator==(const data& in_rhs) const;
  bool operator!=(const data& in_rhs) const;
  friend void to_json(nlohmann::json& j, const data& p) {
    j["path"]    = p.path;
    j["use_sim"] = p.use_sim;
  }
  friend void from_json(const nlohmann::json& j, data& p) {
    p.path    = j.at("path");
    p.use_sim = j.at("use_sim");
  }
};
using data_ptr = std::shared_ptr<data>;
}  // namespace reference_attr

/**
 * @brief 引用文件标签命令
 *
 * @li 添加引用文件解算标签
 * @li 在没有引用文件标记时， 使用所有载入的应用进行解算
 *
 */
class reference_attr_setting : public command_base {
  std::vector<entt::handle> p_handle;
  bool chick_channel() const;
  bool replace_channel_date(const string& in_string) const;
  bool get_file_info();

 public:
  reference_attr_setting();
  bool render() override;
  static string get_channel_date();
};

namespace {
constexpr char create_ref_file_command_name[] = "doodle_create_ref_file";
constexpr char ref_file_load_command_name[]   = "doodle_ref_file_load";
constexpr char ref_file_sim_command_name[]    = "doodle_ref_file_sim";
constexpr char ref_file_export_command_name[] = "doodle_ref_file_export";
}  // namespace
MSyntax create_ref_syntax();
MSyntax ref_file_sim_syntax();
class create_ref_file_command : public MTemplateAction<
                                    create_ref_file_command,
                                    create_ref_file_command_name,
                                    create_ref_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};

class ref_file_load_command : public MTemplateAction<
                                  ref_file_load_command,
                                  ref_file_load_command_name,
                                  MTemplateCommand_nullSyntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
class ref_file_sim_command : public MTemplateAction<
                                 ref_file_sim_command,
                                 ref_file_sim_command_name,
                                 ref_file_sim_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
class ref_file_export_command : public MTemplateAction<
                                    ref_file_export_command,
                                    ref_file_export_command_name,
                                    MTemplateCommand_nullSyntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
}  // namespace doodle::maya_plug
