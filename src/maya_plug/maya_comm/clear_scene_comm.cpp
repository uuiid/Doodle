//
// Created by TD on 2022/2/28.
//

#include "clear_scene_comm.h"

#include <maya/MArgParser.h>
#include <maya/MSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MTime.h>

#include <maya_plug/data/maya_clear_scenes.h>

namespace doodle::maya_plug {
namespace {
constexpr char syntax_select_long[]  = "-select";
constexpr char syntax_select[]       = "-sl";

constexpr char unlock_normal_long[]  = "-unlock_normal";
constexpr char unlock_normal[]       = "-nn";

constexpr char duplicate_name_long[] = "-duplicate_name";
constexpr char duplicate_name[]      = "-dn";

constexpr char multilateral_long[]   = "-multilateral";
constexpr char multilateral[]        = "-mu";

constexpr char uv_set_long[]         = "-uv_set";
constexpr char uv_set[]              = "-uv";

constexpr char err_1_long[]          = "-err_1";
constexpr char err_1[]               = "-e1";

constexpr char err_2_long[]          = "-err_2";
constexpr char err_2[]               = "-e2";

constexpr char err_3_long[]          = "-err_3";
constexpr char err_3[]               = "-e3";

constexpr char err_4_long[]          = "-err_4";
constexpr char err_4[]               = "-e4";
}  // namespace

MSyntax clear_scene_comm_syntax() {
  MSyntax syntax{};
  syntax.addFlag(syntax_select, syntax_select_long);
  syntax.addFlag(unlock_normal, unlock_normal_long);
  syntax.addFlag(duplicate_name, duplicate_name_long);
  syntax.addFlag(multilateral, multilateral_long);
  syntax.addFlag(uv_set, uv_set_long);
  syntax.addFlag(err_1, err_1_long);
  syntax.addFlag(err_2, err_2_long);
  syntax.addFlag(err_3, err_3_long);
  syntax.addFlag(err_4, err_4_long);
  return syntax;
}

MStatus clear_scene_comm::doIt(const MArgList &in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  maya_clear_scenes l_clear_scenes{};

  MSelectionList l_list{};
  std::stringstream k_r{};
  bool hae_err{false};
  if (k_prase.isFlagSet(unlock_normal, &k_s)) {
    DOODLE_MAYA_CHICK(k_s)
    l_clear_scenes.unlock_normal();
    k_r << fmt::format("{} \n", "完成场景中所有多边形解锁法线"s);
  }
  if (k_prase.isFlagSet(duplicate_name, &k_s)) {
    DOODLE_MAYA_CHICK(k_s)
    DOODLE_MAYA_CHICK(l_list.clear());
    if (l_clear_scenes.duplicate_name(l_list)) {
      MStringArray l_m_string_array{};
      DOODLE_MAYA_CHICK(l_list.getSelectionStrings(l_m_string_array));
      k_r << fmt::format("{}\n", "检查到了场景中的网格体重复命名 ") << l_m_string_array << "\n";
      hae_err = true;
    } else
      k_r << fmt::format("{}\n", "没有重名网格");
  }
  if (k_prase.isFlagSet(multilateral, &k_s)) {
    DOODLE_MAYA_CHICK(k_s)
    DOODLE_MAYA_CHICK(l_list.clear())
    if (l_clear_scenes.multilateral_surface(l_list)) {
      MStringArray l_m_string_array{};
      DOODLE_MAYA_CHICK(l_list.getSelectionStrings(l_m_string_array));
      k_r << fmt::format("{}\n", "检查到了场景多边面 ") << l_m_string_array << "\n";
      hae_err = true;
    } else {
      k_r << fmt::format("{}\n", "没有多边面");
    }
  }
  if (k_prase.isFlagSet(uv_set, &k_s)) {
    DOODLE_MAYA_CHICK(k_s)
    DOODLE_MAYA_CHICK(l_list.clear())
    if (l_clear_scenes.uv_set(l_list)) {
      MStringArray l_m_string_array{};
      DOODLE_MAYA_CHICK(l_list.getSelectionStrings(l_m_string_array));
      k_r << fmt::format("{}\n", "检查到了场景多uv集 ") << l_m_string_array << "\n";
      hae_err = true;
    } else {
      k_r << fmt::format("{}\n", "没有多uv集");
    }
  }
  if (k_prase.isFlagSet(err_1, &k_s)) {
    l_clear_scenes.err_1();
    k_r << fmt::format("{}\n", "去除了场景中的 UI outlinerPanel 错误");
    DOODLE_MAYA_CHICK(k_s)
  }
  if (k_prase.isFlagSet(err_2, &k_s)) {
    l_clear_scenes.err_2();
    k_r << fmt::format("{}\n", "去除了场景中的 UI 模型面板回调错误");
    DOODLE_MAYA_CHICK(k_s)
  }
  if (k_prase.isFlagSet(err_3, &k_s)) {
    l_clear_scenes.err_3();
    k_r << fmt::format("{}\n", "去除了场景中的 UI outlinerPanel 错误");
    DOODLE_MAYA_CHICK(k_s)
  }
  if (k_prase.isFlagSet(err_4, &k_s)) {
    l_clear_scenes.err_4();
    k_r << fmt::format("{}\n", "去除了场景中的 未知插件和贼健康错误");
    DOODLE_MAYA_CHICK(k_s)
  }

  if (k_prase.isFlagSet(syntax_select, &k_s)) {
    DOODLE_MAYA_CHICK(k_s)
    DOODLE_MAYA_CHICK(MGlobal::setActiveSelectionList(l_list));
  }
  MStringArray l_array{2, MString{}};
  l_array[0] = hae_err ? "1" : "0";
  l_array[1] = d_str{k_r.str()};
  setResult(l_array);
  return k_s;
}
bool clear_scene_comm::show_save_mag() {
  MTime l_start{};
  l_start.setValue(1001);
  std::vector<std::string> l_msg{
      "请检查一下几项:"s,
      ""s,
      fmt::format(R"(开始帧为 1001  --> {})",
                  MAnimControl::minTime() == l_start ? "正确"s : "错误"s),
      fmt::format(R"(总帧数为       --> {} 请确认)",
                  (MAnimControl::maxTime() - MAnimControl::minTime()).value() + 1),
      "5. 帧率(25)"s,
      "1. 检查950帧 TPost"s,
      "2. 摄像机: 命名(项目缩写_ep集数_sc镜头_开始帧_结束帧), 是否有多余"s,
      "3. 摄像机命名"s,
      "4. 多余的引用角色"s,
      "6. 相机名称和结束帧"s};

  MString l_com_r{};
  MGlobal::executeCommand(d_str{fmt::format(R"(
confirmDialog
-button "保存"
-button "No"
-defaultButton "保存"
-cancelButton "不保存"
-dismissString "不保存"
-icon "warning"
-message "{}"
;
)"s,
                                            fmt::join(l_msg, R"(\n)"))},
                          l_com_r, false, false);
  return l_com_r == d_str{"保存"};
}
}  // namespace doodle::maya_plug
