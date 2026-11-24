//
// Created by TD on 2026/11/16.
//

#include "face_capture_import.h"

#include "maya_plug_fwd.h"
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/maya_display.h>
#include <maya_plug/data/maya_tool.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MNamespace.h>
#include <maya/MObjectArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace doodle::maya_plug {

namespace {

constexpr char k_curves_magic[]{"DHFC0001"};
constexpr std::uint32_t k_curves_version{1};
constexpr std::int32_t k_curve_name_length{64};
constexpr std::int32_t k_curves_header_size{48};
constexpr float k_seg_epsilon{1e-4f};
constexpr double k_fps_epsilon{1e-6};
constexpr char k_target_control_name[]{"CTRL_expressions"};
constexpr std::string_view k_raw_prefix{"CTRL_expressions_"};

struct face_capture_map_seg_t {
  float from_{};
  float to_{};
  float slope_{};
  float cut_{};
};

struct face_capture_map_binding_t {
  std::size_t gui_index_{};
  face_capture_map_seg_t seg_{};
};

struct face_capture_gui_def_t {
  std::string gui_name_{};
  std::string node_name_{};
  std::string attr_name_{};
  std::vector<std::pair<std::string, face_capture_map_seg_t>> raw_entries_{};
};

struct face_capture_raw_binding_t {
  std::string raw_name_{};
  std::vector<face_capture_map_binding_t> bindings_{};
};

#include "face_capture_curve_map.inc"

const std::unordered_map<std::string, const face_capture_raw_binding_t*> k_raw_lookup = []() {
  std::unordered_map<std::string, const face_capture_raw_binding_t*> l_map{};
  l_map.reserve(k_face_capture_raw_bindings.size());
  for (const auto& l_binding : k_face_capture_raw_bindings) {
    l_map.emplace(l_binding.raw_name_, &l_binding);
  }
  return l_map;
}();

struct face_capture_file_t {
  std::uint32_t frame_count_{};
  std::uint32_t curve_count_{};
  std::uint32_t fps_num_{};
  std::uint32_t fps_den_{};
  std::vector<std::string> curve_names_{};
  std::vector<float> values_{};
  std::unordered_map<std::string, std::size_t> curve_index_{};
};

MString dag_leaf_of(const MString& in_dag_path) {
  const int l_bar = in_dag_path.rindex('|');
  return l_bar < 0 ? in_dag_path : MString{in_dag_path.substring(l_bar + 1, in_dag_path.length())};
}

MString namespace_of(const MDagPath& in_path) { return MNamespace::getNamespaceFromName(dag_leaf_of(in_path.fullPathName())); }

MString leaf_name_of_dag_path(const MDagPath& in_path) {
  return MNamespace::stripNamespaceFromName(dag_leaf_of(in_path.fullPathName()));
}

std::string full_path_of(const MDagPath& in_path) {
  MString l_full = in_path.fullPathName();
  return conv::to_s(l_full);
}

std::uint32_t read_u32_le(const std::uint8_t* in_ptr) {
  return static_cast<std::uint32_t>(in_ptr[0]) | (static_cast<std::uint32_t>(in_ptr[1]) << 8) |
         (static_cast<std::uint32_t>(in_ptr[2]) << 16) | (static_cast<std::uint32_t>(in_ptr[3]) << 24);
}

float read_f32_le(const std::uint8_t* in_ptr) {
  const std::uint32_t l_bits = read_u32_le(in_ptr);
  float l_value{};
  std::memcpy(&l_value, &l_bits, sizeof(float));
  return l_value;
}

std::vector<std::uint8_t> read_all_bytes(const FSys::path& in_path) {
  std::ifstream l_stream{in_path, std::ios::binary | std::ios::ate};
  if (!l_stream) return {};
  const auto l_size = l_stream.tellg();
  if (l_size <= 0) return {};
  std::vector<std::uint8_t> l_bytes(static_cast<std::size_t>(l_size));
  l_stream.seekg(0, std::ios::beg);
  l_stream.read(reinterpret_cast<char*>(l_bytes.data()), static_cast<std::streamsize>(l_size));
  return l_bytes;
}

std::string trim_curve_name(const std::uint8_t* in_ptr) {
  std::string l_name{};
  for (std::int32_t l_i = 0; l_i < k_curve_name_length; ++l_i) {
    const char l_ch = static_cast<char>(in_ptr[l_i]);
    if (l_ch == '\0') break;
    l_name.push_back(l_ch);
  }
  return l_name;
}

struct gui_node_lookup_t {
  MObject node_{};
  std::size_t count_{};
};

using gui_node_index_t = std::unordered_map<std::string, std::vector<MObject>>;

gui_node_index_t build_gui_node_index(const MObjectArray& in_objects) {
  gui_node_index_t l_index{};
  l_index.reserve(in_objects.length());
  for (unsigned l_i = 0; l_i < in_objects.length(); ++l_i) {
    MDagPath l_path{};
    if (MDagPath::getAPathTo(in_objects[l_i], l_path) != MS::kSuccess) continue;
    l_index[conv::to_s(leaf_name_of_dag_path(l_path))].push_back(in_objects[l_i]);
  }
  return l_index;
}

gui_node_lookup_t lookup_gui_node(const gui_node_index_t& in_index, const std::string& in_node_name) {
  auto l_iter = in_index.find(in_node_name);
  if (l_iter == in_index.end()) return {};
  if (l_iter->second.size() != 1) return {.count_ = l_iter->second.size()};
  return {.node_ = l_iter->second.front(), .count_ = 1};
}

std::string raw_key_of(std::string_view in_name) {
  std::string l_name{in_name};
  const auto l_pos = l_name.find_last_of(':');
  if (l_pos != std::string::npos) l_name.erase(0, l_pos + 1);
  if (l_name.rfind(k_raw_prefix, 0) == 0) l_name.erase(0, k_raw_prefix.size());
  return l_name;
}

struct gui_candidate_t {
  float value_{};
  std::size_t count_{};
};

void solve_gui_value(gui_candidate_t& in_out, float in_raw, const face_capture_map_seg_t& in_seg) {
  if (std::abs(in_seg.slope_) < k_seg_epsilon) return;
  const float l_gui = (in_raw - in_seg.cut_) / in_seg.slope_;
  if (l_gui < in_seg.from_ - k_seg_epsilon || l_gui > in_seg.to_ + k_seg_epsilon) return;
  in_out.value_ += l_gui;
  ++in_out.count_;
}

bool check_gui_has_input(MObject in_node_object, const std::string& in_attr_name, std::string& in_out_source) {
  MFnDependencyNode l_dep{in_node_object};
  MObject l_attr = l_dep.attribute(conv::to_ms(in_attr_name));
  if (l_attr.isNull()) return false;

  MPlug l_plug{in_node_object, l_attr};
  MPlugArray l_sources{};
  l_plug.connectedTo(l_sources, true, false);
  if (l_sources.length() == 0) return false;

  MObject l_source_attr = l_sources[0].attribute();
  MFnDependencyNode l_source_node{l_sources[0].node()};
  in_out_source = fmt::format("{}.{}", conv::to_s(l_source_node.name()), conv::to_s(l_source_attr.apiTypeStr()));
  return true;
}

}  // namespace

MSyntax face_capture_import_syntax() {
  MSyntax l_syntax{};
  CHECK_MSTATUS(l_syntax.addFlag("-fp", "-file_path", MSyntax::kString));
  CHECK_MSTATUS(l_syntax.setObjectType(MSyntax::kSelectionList, 1, 1));
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

class face_capture_import::impl {
 public:
  std::string file_path_{};
  MDagPath node_path_{};

  face_capture_file_t file_{};
  MDagPath target_path_{};
  MString target_namespace_{};
  gui_node_index_t gui_node_index_{};
  std::vector<std::size_t> raw_resolved_{};

  MStatus load_file();
  MStatus locate_target();
  MStatus precheck_frame_rate();
  MStatus precheck_gui_connections();
  MStatus import_curves();
};

MStatus face_capture_import::impl::load_file() {
  const FSys::path l_path{file_path_};
  if (!FSys::exists(l_path)) return display_error("面捕曲线文件不存在: {}", file_path_), MS::kFailure;

  const auto l_bytes = read_all_bytes(l_path);
  if (l_bytes.size() < static_cast<std::size_t>(k_curves_header_size)) {
    return display_error("面捕曲线文件过短, 不足 {} 字节: {}", k_curves_header_size, file_path_), MS::kFailure;
  }

  if (std::memcmp(l_bytes.data(), k_curves_magic, 8) != 0) {
    return display_error("不是合法的 .dhfc 文件(magic 不匹配): {}", file_path_), MS::kFailure;
  }

  const auto l_version = read_u32_le(l_bytes.data() + 8);
  if (l_version != k_curves_version) {
    return display_error("不支持的 .dhfc 版本: {} (期望 {})", l_version, k_curves_version), MS::kFailure;
  }

  file_.frame_count_ = read_u32_le(l_bytes.data() + 16);
  file_.curve_count_ = read_u32_le(l_bytes.data() + 20);
  file_.fps_num_     = read_u32_le(l_bytes.data() + 24);
  file_.fps_den_     = read_u32_le(l_bytes.data() + 28);

  if (file_.fps_num_ == 0 || file_.fps_den_ == 0) {
    return display_error("面捕曲线的帧率非法: {}/{}", file_.fps_num_, file_.fps_den_), MS::kFailure;
  }
  if (file_.frame_count_ == 0 || file_.curve_count_ == 0) {
    return display_error("面捕曲线为空: frame_count={} curve_count={}", file_.frame_count_, file_.curve_count_),
           MS::kFailure;
  }

  const std::size_t l_name_begin  = k_curves_header_size;
  const std::size_t l_name_bytes  = static_cast<std::size_t>(k_curve_name_length) * file_.curve_count_;
  const std::size_t l_value_begin = l_name_begin + l_name_bytes;
  const std::size_t l_value_bytes = static_cast<std::size_t>(sizeof(float)) * file_.frame_count_ * file_.curve_count_;
  if (l_bytes.size() < l_value_begin + l_value_bytes) {
    return display_error("面捕曲线文件被截断: 需要 {} 字节, 实际 {}", l_value_begin + l_value_bytes, l_bytes.size()),
           MS::kFailure;
  }

  file_.curve_names_.reserve(file_.curve_count_);
  for (std::uint32_t l_i = 0; l_i < file_.curve_count_; ++l_i) {
    auto l_name = trim_curve_name(l_bytes.data() + l_name_begin + static_cast<std::size_t>(l_i) * k_curve_name_length);
    file_.curve_index_.emplace(l_name, file_.curve_names_.size());
    file_.curve_names_.push_back(std::move(l_name));
  }

  file_.values_.resize(static_cast<std::size_t>(file_.frame_count_) * file_.curve_count_);
  for (std::size_t l_i = 0; l_i < file_.values_.size(); ++l_i) {
    file_.values_[l_i] = read_f32_le(l_bytes.data() + l_value_begin + l_i * sizeof(float));
  }

  return MS::kSuccess;
}

MStatus face_capture_import::impl::locate_target() {
  if (!node_path_.isValid() || node_path_.node().isNull()) return display_error("未选择有效的导入目标节点"), MS::kFailure;
  target_path_ = node_path_;
  target_namespace_ = namespace_of(target_path_);
  const MObjectArray l_objects = MNamespace::getNamespaceObjects(target_namespace_);
  gui_node_index_              = build_gui_node_index(l_objects);

  MDagPathArray l_candidates{};

  auto l_push = [&](const MDagPath& in_path) {
    for (unsigned l_i = 0; l_i < l_candidates.length(); ++l_i) {
      if (l_candidates[l_i] == in_path) return;
    }
    l_candidates.append(in_path);
  };

  std::string l_suggestion{};
  for (unsigned l_i = 0; l_i < l_objects.length(); ++l_i) {
    MDagPath l_path{};
    if (MDagPath::getAPathTo(l_objects[l_i], l_path) != MS::kSuccess) continue;
    const auto l_leaf = conv::to_s(leaf_name_of_dag_path(l_path));
    if (l_leaf == k_target_control_name) {
      l_push(l_path);
      continue;
    }
    if (l_leaf.find("CTRL_expr") == std::string::npos) continue;
    if (!l_suggestion.empty()) l_suggestion += ", ";
    l_suggestion += l_leaf;
  }

  if (l_candidates.length() == 0) {
    const std::string l_ns = target_namespace_.length() == 0 ? std::string{"(root)"} : conv::to_s(target_namespace_);
    return display_error(
               "在命名空间 \"{}\" 下找不到 {} 节点{}", l_ns, k_target_control_name,
               l_suggestion.empty() ? std::string{} : fmt::format(", 相近节点: {}", l_suggestion)
           ),
           MS::kFailure;
  }

  if (l_candidates.length() > 1) {
    std::string l_list{};
    for (unsigned l_i = 0; l_i < l_candidates.length(); ++l_i) {
      if (!l_list.empty()) l_list += ", ";
      l_list += full_path_of(l_candidates[l_i]);
    }
    const std::string l_ns = target_namespace_.length() == 0 ? std::string{"(root)"} : conv::to_s(target_namespace_);
    return display_error(
               "命名空间 \"{}\" 下找到 {} 个 {}: {}, 无法确定目标", l_ns, l_candidates.length(), k_target_control_name,
               l_list
           ),
           MS::kFailure;
  }

  target_path_ = l_candidates[0];
  return MS::kSuccess;
}

MStatus face_capture_import::impl::precheck_frame_rate() {
  const double l_file_fps = static_cast<double>(file_.fps_num_) / static_cast<double>(file_.fps_den_);
  MTime l_one_second{1.0, MTime::kSeconds};
  const double l_scene_fps = l_one_second.as(MTime::uiUnit());
  if (l_scene_fps <= 0.0) return display_error("无法读取场景帧率"), MS::kFailure;

  if (std::abs(l_file_fps - l_scene_fps) > k_fps_epsilon * std::max(1.0, std::abs(l_scene_fps))) {
    return display_error(
               "帧率不一致: 面捕曲线 {}/{} = {:.6f}, 场景 {:.6f}, 请先统一帧率再导入", file_.fps_num_, file_.fps_den_,
               l_file_fps, l_scene_fps
           ),
           MS::kFailure;
  }
  return MS::kSuccess;
}

MStatus face_capture_import::impl::precheck_gui_connections() {
  std::vector<std::string> l_conflicts{};

  for (const auto& l_gui : k_face_capture_gui_map) {
    MObject l_node_object = lookup_gui_node(gui_node_index_, l_gui.node_name_).node_;
    if (l_node_object.isNull()) continue;
    std::string l_source{};
    if (!check_gui_has_input(l_node_object, l_gui.attr_name_, l_source)) continue;
    l_conflicts.push_back(fmt::format("{}({})", l_gui.gui_name_, l_source));
  }

  if (!l_conflicts.empty()) {
    std::string l_list{};
    for (std::size_t l_i = 0; l_i < l_conflicts.size() && l_i < 5; ++l_i) {
      if (!l_list.empty()) l_list += ", ";
      l_list += l_conflicts[l_i];
    }
    return display_error(
               "GUI 控制器已有上游输入连接, 拒绝导入 (共 {} 个, 前 5 个): {}", l_conflicts.size(), l_list
           ),
           MS::kFailure;
  }
  return MS::kSuccess;
}

MStatus face_capture_import::impl::import_curves() {
  const std::size_t l_curve_count = file_.curve_count_;
  const std::size_t l_frame_count = file_.frame_count_;

  std::vector<std::size_t> l_curve_raw_index(l_curve_count, 0);
  std::size_t l_resolved{0};
  std::vector<std::string> l_skipped{};
  for (std::size_t l_i = 0; l_i < l_curve_count; ++l_i) {
    const auto l_key = raw_key_of(file_.curve_names_[l_i]);
    auto l_iter      = k_raw_lookup.find(l_key);
    if (l_iter == k_raw_lookup.end()) {
      l_skipped.push_back(file_.curve_names_[l_i]);
      continue;
    }
    l_curve_raw_index[l_i] = l_iter->second - k_face_capture_raw_bindings.data();
    ++l_resolved;
  }

  if (!l_skipped.empty()) {
    std::string l_list{};
    for (std::size_t l_i = 0; l_i < l_skipped.size() && l_i < 5; ++l_i) {
      if (!l_list.empty()) l_list += ", ";
      l_list += l_skipped[l_i];
    }
    display_warning("以下曲线在映射表中不存在, 已跳过 (共 {} 条): {}", l_skipped.size(), l_list);
  }
  if (l_resolved == 0) return display_error("没有任何曲线能对应到映射表, 中止导入"), MS::kFailure;

  const std::size_t l_gui_count = k_face_capture_gui_map.size();
  std::vector<std::vector<float>> l_gui_values(l_gui_count, std::vector<float>(l_frame_count, 0.0f));
  std::vector<bool> l_gui_written(l_gui_count, false);

  std::unordered_map<std::string, float> l_raw_values{};
  l_raw_values.reserve(l_resolved);
  for (std::uint32_t l_frame = 0; l_frame < l_frame_count; ++l_frame) {
    l_raw_values.clear();
    for (std::size_t l_i = 0; l_i < l_curve_count; ++l_i) {
      l_raw_values.emplace(
          raw_key_of(file_.curve_names_[l_i]), file_.values_[static_cast<std::size_t>(l_frame) * l_curve_count + l_i]
      );
    }

    for (std::size_t l_gui_index = 0; l_gui_index < l_gui_count; ++l_gui_index) {
      const auto& l_gui = k_face_capture_gui_map[l_gui_index];
      gui_candidate_t l_candidate{};
      for (const auto& [l_raw_name, l_seg] : l_gui.raw_entries_) {
        auto l_iter = l_raw_values.find(l_raw_name);
        if (l_iter == l_raw_values.end()) continue;
        solve_gui_value(l_candidate, l_iter->second, l_seg);
      }
      if (l_candidate.count_ == 0) continue;
      l_gui_values[l_gui_index][l_frame] = l_candidate.value_ / static_cast<float>(l_candidate.count_);
      l_gui_written[l_gui_index]         = true;
    }
  }

  MTime l_one_second{1.0, MTime::kSeconds};
  const double l_scene_fps = l_one_second.as(MTime::uiUnit());
  const MTime l_start_time = MAnimControl::minTime();

  std::size_t l_created{0};
  std::vector<std::string> l_create_failed{};

  for (std::size_t l_gui_index = 0; l_gui_index < l_gui_count; ++l_gui_index) {
    if (!l_gui_written[l_gui_index]) continue;
    const auto& l_gui = k_face_capture_gui_map[l_gui_index];

    MObject l_node_object = lookup_gui_node(gui_node_index_, l_gui.node_name_).node_;
    if (l_node_object.isNull()) {
      l_create_failed.push_back(l_gui.gui_name_);
      continue;
    }

    MFnDependencyNode l_dep{l_node_object};
    MObject l_attr = l_dep.attribute(conv::to_ms(l_gui.attr_name_));
    if (l_attr.isNull()) {
      l_create_failed.push_back(l_gui.gui_name_);
      continue;
    }

    MPlug l_plug{l_node_object, l_attr};
    MFnAnimCurve l_curve{};
    MStatus l_status = MS::kSuccess;
    l_curve.create(l_plug, MFnAnimCurve::kAnimCurveTL, nullptr, &l_status);
    if (!l_status) {
      l_create_failed.push_back(l_gui.gui_name_);
      continue;
    }

    MTimeArray l_times{};
    MDoubleArray l_values{};
    for (std::uint32_t l_frame = 0; l_frame < l_frame_count; ++l_frame) {
      l_times.append(l_start_time + MTime{static_cast<double>(l_frame) / l_scene_fps, MTime::kSeconds});
      l_values.append(static_cast<double>(l_gui_values[l_gui_index][l_frame]));
    }
    l_status = l_curve.addKeys(&l_times, &l_values, MFnAnimCurve::kTangentLinear, MFnAnimCurve::kTangentLinear);
    if (!l_status) {
      l_create_failed.push_back(l_gui.gui_name_);
      continue;
    }

    ++l_created;
  }

  if (!l_create_failed.empty()) {
    std::string l_list{};
    for (std::size_t l_i = 0; l_i < l_create_failed.size() && l_i < 5; ++l_i) {
      if (!l_list.empty()) l_list += ", ";
      l_list += l_create_failed[l_i];
    }
    display_warning("以下 GUI 控制器写入失败 (共 {} 个): {}", l_create_failed.size(), l_list);
  }

  display_info(
      "面捕曲线导入完成: 写入 {} 个 GUI 控制器, {} 帧, 目标 {} [命名空间 {}]", l_created, l_frame_count,
      full_path_of(target_path_), target_namespace_.length() == 0 ? std::string{"(root)"} : conv::to_s(target_namespace_)
  );

  if (l_created == 0) return display_error("没有写入任何 GUI 控制器"), MS::kFailure;
  return MS::kSuccess;
}

face_capture_import::face_capture_import() : p_i(std::make_unique<impl>()) {}
face_capture_import::~face_capture_import() = default;

MStatus face_capture_import::get_arg(const MArgList& in_arg) {
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_arg, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

  if (l_arg_data.isFlagSet("-fp")) {
    MString l_file_path;
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_arg_data.getFlagArgument("-fp", 0, l_file_path));
    p_i->file_path_ = conv::to_s(l_file_path);
  } else {
    return display_error("缺少参数 -fp <file_path>"), MS::kFailure;
  }

  MSelectionList l_selection{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_arg_data.getObjects(l_selection));
  if (l_selection.length() != 1) return display_error("节点参数数量不是 1 (当前 {})", l_selection.length()), MS::kFailure;
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_selection.getDagPath(0, p_i->node_path_));

  return MS::kSuccess;
}

MStatus face_capture_import::doIt(const MArgList& in_list) {
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_arg(in_list));

  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->load_file());
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->locate_target());
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->precheck_frame_rate());
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->precheck_gui_connections());
  return p_i->import_curves();
}

}  // namespace doodle::maya_plug
