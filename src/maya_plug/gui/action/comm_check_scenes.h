//
// Created by TD on 2021/11/2.
//

#include <doodle_app/gui/base/base_window.h>

#include <maya_plug/configure/static_value.h>

#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
/**
 * @brief maya检查场景功能
 *
 * @li 检查maya场景一共有一下几项
 * * 检查所有
 * * 解锁法线
 * * 检查重名
 * * 检查大于四边面
 * * 检查UV集
 * * 去除大纲错误
 * * 去除onModelChange3dc错误
 * * 去除CgAbBlastPanelOptChangeCallback错误
 * * 去除贼健康错误
 *
 */
class comm_check_scenes {
  bool p_unlock_normal;
  bool p_duplicate_name;
  bool p_multilateral_surface;
  bool p_uv_set;
  bool p_err_1;
  bool p_err_2;
  bool p_err_3;
  bool p_err_4;
  std::string title_name_;
  bool open{true};

  static MStatus run_maya_py_script(const std::string& in_script);

  MStatus unlock_normal();
  MStatus duplicate_namel(bool use_select);
  MStatus multilateral_surface(bool use_select);
  MStatus uv_set(bool use_select);
  MStatus err_1();  // (1)大纲
  MStatus err_2();  // (2)onModelChange3dc
  MStatus err_3();  // (3)CgAbBlastPanelOptChangeCallback
  MStatus err_4();  // (4)贼健康

 public:
  comm_check_scenes();
  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::comm_check_scenes;
  const std::string& title() const;
  bool render();
};

}  // namespace doodle::maya_plug
