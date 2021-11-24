//
// Created by TD on 2021/11/2.
//

#include <doodle_lib/gui/action/command.h>
#include <maya/MStatus.h>
#include <maya/MSelectionList.h>
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
class comm_check_scenes : public command_base {
  bool p_unlock_normal;
  bool p_duplicate_name;
  bool p_multilateral_surface;
  bool p_uv_set;
  bool p_err_1;
  bool p_err_2;
  bool p_err_3;
  bool p_err_4;

  static MStatus run_maya_py_script(const string& in_script);

  MStatus unlock_normal();
  MStatus duplicate_namel(bool use_select);
  MStatus multilateral_surface(bool use_select);
  MStatus uv_set(bool use_select);
  MStatus err_1();  // (1)大纲
  MStatus err_2();  // (2)onModelChange3dc
  MStatus err_3();  // (3)CgAbBlastPanelOptChangeCallback
  MStatus err_4();  // (4)贼健康

  MStatus print_mfn();
 public:
  comm_check_scenes();
  virtual bool render() override;
};
}  // namespace doodle::maya_plug
