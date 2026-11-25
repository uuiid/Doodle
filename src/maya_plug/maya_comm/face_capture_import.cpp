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
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MNamespace.h>
#include <maya/MObjectArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
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
constexpr double k_fps_epsilon{1e-6};

struct face_capture_channel_t {
  std::string node_name_{};
  std::string attr_name_{};
};

struct face_capture_file_t {
  std::uint32_t frame_count_{};
  std::uint32_t curve_count_{};
  std::uint32_t fps_num_{};
  std::uint32_t fps_den_{};
  std::vector<std::string> curve_names_{};
  std::vector<face_capture_channel_t> channels_{};
  std::vector<float> values_{};
};

MString dag_leaf_of(const MString& in_dag_path) {
  const int l_bar = in_dag_path.rindex('|');
  return l_bar < 0 ? in_dag_path : MString{in_dag_path.substring(l_bar + 1, in_dag_path.length())};
}

MString namespace_of(const MDagPath& in_path) {
  return MNamespace::getNamespaceFromName(dag_leaf_of(in_path.fullPathName()));
}

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

std::optional<face_capture_channel_t> parse_channel_name(const std::string& in_name) {
  const auto l_dot = in_name.rfind('.');
  if (l_dot == std::string::npos || l_dot == 0 || l_dot + 1 >= in_name.size()) return std::nullopt;

  face_capture_channel_t l_channel{};
  l_channel.node_name_ = in_name.substr(0, l_dot);
  l_channel.attr_name_ = in_name.substr(l_dot + 1);
  return l_channel;
}

struct node_index_t {
  std::unordered_map<std::string, MObject> node_{};
  std::unordered_map<std::string, std::size_t> count_{};
};

node_index_t build_node_index(const MObjectArray& in_objects) {
  node_index_t l_index{};
  for (unsigned l_i = 0; l_i < in_objects.length(); ++l_i) {
    MDagPath l_path{};
    if (MDagPath::getAPathTo(in_objects[l_i], l_path) != MS::kSuccess) continue;
    const auto l_leaf = conv::to_s(leaf_name_of_dag_path(l_path));
    l_index.node_[l_leaf] = in_objects[l_i];
    ++l_index.count_[l_leaf];
  }
  return l_index;
}

}  // namespace

MSyntax face_capture_import_syntax() {
  MSyntax l_syntax{};
  CHECK_MSTATUS(l_syntax.addFlag("-fp", "-file_path", MSyntax::kString));
  CHECK_MSTATUS(l_syntax.setObjectType(MSyntax::kSelectionList, 1));
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
  node_index_t node_index_{};

  MStatus load_file();
  MStatus locate_target();
  MStatus precheck_frame_rate();
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
  file_.channels_.reserve(file_.curve_count_);
  for (std::uint32_t l_i = 0; l_i < file_.curve_count_; ++l_i) {
    auto l_name = trim_curve_name(l_bytes.data() + l_name_begin + static_cast<std::size_t>(l_i) * k_curve_name_length);
    auto l_channel = parse_channel_name(l_name);
    if (!l_channel) {
      return display_error("面捕通道名不合法(应为 <控制器>.<通道>): {}", l_name), MS::kFailure;
    }
    file_.curve_names_.push_back(std::move(l_name));
    file_.channels_.push_back(std::move(*l_channel));
  }

  file_.values_.resize(static_cast<std::size_t>(file_.frame_count_) * file_.curve_count_);
  for (std::size_t l_i = 0; l_i < file_.values_.size(); ++l_i) {
    file_.values_[l_i] = read_f32_le(l_bytes.data() + l_value_begin + l_i * sizeof(float));
  }

  return MS::kSuccess;
}

MStatus face_capture_import::impl::locate_target() {
  if (!node_path_.isValid() || node_path_.node().isNull()) return display_error("未选择有效的导入目标节点"), MS::kFailure;

  target_path_      = node_path_;
  target_namespace_ = namespace_of(target_path_);
  node_index_       = build_node_index(MNamespace::getNamespaceObjects(target_namespace_));

  if (node_index_.node_.empty()) {
    const std::string l_ns = target_namespace_.length() == 0 ? std::string{"(root)"} : conv::to_s(target_namespace_);
    return display_error("命名空间 \"{}\" 下没有任何节点", l_ns), MS::kFailure;
  }

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

MStatus face_capture_import::impl::import_curves() {
  MTime l_one_second{1.0, MTime::kSeconds};
  const double l_scene_fps = l_one_second.as(MTime::uiUnit());
  const MTime l_start_time = MAnimControl::minTime();

  std::size_t l_created{0};
  std::vector<std::string> l_create_failed{};

  for (std::uint32_t l_index = 0; l_index < file_.curve_count_; ++l_index) {
    const auto& l_channel = file_.channels_[l_index];
    const auto& l_name    = file_.curve_names_[l_index];

    auto l_node_iter = node_index_.node_.find(l_channel.node_name_);
    if (l_node_iter == node_index_.node_.end()) {
      l_create_failed.push_back(fmt::format("{}: 找不到节点", l_name));
      continue;
    }
    const auto l_count_iter = node_index_.count_.find(l_channel.node_name_);
    if (l_count_iter != node_index_.count_.end() && l_count_iter->second != 1) {
      l_create_failed.push_back(fmt::format("{}: 节点重名 {} 个", l_name, l_count_iter->second));
      continue;
    }

    MFnDependencyNode l_dep{l_node_iter->second};
    MObject l_attr = l_dep.attribute(conv::to_ms(l_channel.attr_name_));
    if (l_attr.isNull()) {
      l_create_failed.push_back(fmt::format("{}: 节点上没有该属性", l_name));
      continue;
    }

    MPlug l_plug{l_node_iter->second, l_attr};
    MFnAnimCurve l_curve{};
    MStatus l_status = MS::kSuccess;
    l_curve.create(l_plug, MFnAnimCurve::kAnimCurveTL, nullptr, &l_status);
    if (!l_status) {
      l_create_failed.push_back(fmt::format("{}: 创建动画曲线失败", l_name));
      continue;
    }

    MTimeArray l_times{};
    MDoubleArray l_values{};
    for (std::uint32_t l_frame = 0; l_frame < file_.frame_count_; ++l_frame) {
      l_times.append(l_start_time + MTime{static_cast<double>(l_frame) / l_scene_fps, MTime::kSeconds});
      l_values.append(static_cast<double>(file_.values_[static_cast<std::size_t>(l_frame) * file_.curve_count_ + l_index]));
    }

    l_status = l_curve.addKeys(&l_times, &l_values, MFnAnimCurve::kTangentLinear, MFnAnimCurve::kTangentLinear);
    if (!l_status) {
      l_create_failed.push_back(fmt::format("{}: 写入关键帧失败", l_name));
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
    display_warning("以下通道写入失败 (共 {} 个): {}", l_create_failed.size(), l_list);
  }

  display_info(
      "面捕曲线导入完成: 写入 {} 条控制器通道, {} 帧, 目标 {} [命名空间 {}]", l_created, file_.frame_count_,
      full_path_of(target_path_), target_namespace_.length() == 0 ? std::string{"(root)"} : conv::to_s(target_namespace_)
  );

  if (l_created == 0) return display_error("没有写入任何控制器通道"), MS::kFailure;
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
  if (l_selection.length() == 0) return display_error("未选择导入目标节点"), MS::kFailure;
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_selection.getDagPath(0, p_i->node_path_));

  return MS::kSuccess;
}

MStatus face_capture_import::doIt(const MArgList& in_list) {
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_arg(in_list));

  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->load_file());
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->locate_target());
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(p_i->precheck_frame_rate());
  return p_i->import_curves();
}

}  // namespace doodle::maya_plug
