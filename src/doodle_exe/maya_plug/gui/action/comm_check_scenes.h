//
// Created by TD on 2021/11/2.
//

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
class comm_check_scenes : public process_t<comm_check_scenes> {
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

 public:
  comm_check_scenes();
  constexpr static std::string_view name{"检查工具"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
  virtual bool render();
};
}  // namespace doodle::maya_plug
