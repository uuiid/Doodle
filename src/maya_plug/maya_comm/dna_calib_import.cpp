#include "dna_calib_import.h"

#include "doodle_core/exception/exception.h"

#include <maya_plug/node/dna_calib_node.h>

#include "data/maya_conv_str.h"
#include "data/maya_tool.h"
#include <dnacalib/DNACalib.h>
#include <fmt/format.h>
#include <maya/MArgDatabase.h>
#include <maya/MFloatArray.h>
#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <numeric>
#include <pma/ScopedPtr.h>
#include <vector>

namespace fmt {
// fmt dna StringView
template <>
struct formatter<dna::StringView> : formatter<string_view> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.end(); }

  template <typename FormatContext>
  auto format(const dna::StringView& sv, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(std::string_view{sv.data(), sv.size()}, ctx);
  }
};

}  // namespace fmt

namespace doodle::maya_plug {
MSyntax dna_calib_import_syntax() {
  MSyntax l_syntax{};
  CHECK_MSTATUS(l_syntax.setObjectType(MSyntax::kSelectionList, 1, 1));
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

class dna_calib_import::impl {
 public:
  FSys::path file_path;
  MObject dna_node_obj{};

  MStatus import_dna_calib() {
    auto l_dna_file = dnac::makeScoped<dnac::FileStream>(
        file_path.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
    );
    auto l_render = dnac::makeScoped<dnac::BinaryStreamReader>(l_dna_file.get());
    l_render->read();
    if (!dnac::Status::isOk())
      return MGlobal::displayError(conv::to_ms(fmt::format("读取dna文件失败: {} ", dnac::Status::get().message))),
             MS::kFailure;

    auto l_dna_render = dnac::makeScoped<dnac::DNACalibDNAReader>(l_render.get());
    for (auto i = 0; i < l_dna_render->getMeshCount(); ++i) {
      auto l_name         = l_dna_render->getMeshName(i);

      auto l_vertex_count = l_dna_render->getVertexPositionCount(i);
      if (l_vertex_count == 0) continue;
      auto l_uv_count     = l_dna_render->getVertexTextureCoordinateCount(i);
      auto l_layout_count = l_dna_render->getVertexLayoutCount(i);
      auto l_face_count   = l_dna_render->getFaceCount(i);
      MGlobal::displayInfo(
          conv::to_ms(
              fmt::format(
                  "Mesh {}: 名称: {} 顶点数量: {} uv数量: {} layout数量: {}, 面数量: {}", i, l_name, l_vertex_count,
                  l_uv_count, l_layout_count, l_face_count
              )
          )
      );
      MPointArray l_vertices{};
      l_vertices.setLength(static_cast<unsigned int>(l_vertex_count));
      for (auto j = 0; j < l_vertex_count; ++j) {
        auto l_pos = l_dna_render->getVertexPosition(i, j);
        l_vertices.set(static_cast<unsigned int>(j), l_pos.x, l_pos.y, l_pos.z);
      }
      std::vector<dna::VertexLayout> l_layouts{};
      l_layouts.reserve(static_cast<size_t>(l_layout_count));
      for (auto j = 0; j < l_layout_count; ++j) {
        auto l_layout = l_dna_render->getVertexLayout(i, j);
        l_layouts.push_back(l_layout);
      }

      MFloatArray l_u_array{}, l_v_array{};
      l_u_array.setLength(static_cast<unsigned int>(l_uv_count));
      l_v_array.setLength(static_cast<unsigned int>(l_uv_count));
      for (auto j = 0; j < l_uv_count; ++j) {
        auto l_uv = l_dna_render->getVertexTextureCoordinate(i, j);
        l_u_array.set(l_uv.u, static_cast<unsigned int>(j));
        l_v_array.set(l_uv.v, static_cast<unsigned int>(j));
      }

      MIntArray l_face_vertex_counts{};
      MIntArray l_face_vertex_indices{};
      l_face_vertex_indices.setSizeIncrement(l_face_count);
      l_face_vertex_counts.setLength(static_cast<unsigned int>(l_face_count));
      for (auto j = 0; j < l_face_count; ++j) {
        auto l_indices = l_dna_render->getFaceVertexLayoutIndices(i, j);
        l_face_vertex_counts.set(static_cast<int>(l_indices.size()), static_cast<unsigned int>(j));
        for (auto k = 0; k < l_indices.size(); ++k) {
          l_face_vertex_indices.append(static_cast<int>(l_layouts[l_indices[k]].position));
        }
      }

      MFnMesh l_fn_mesh{};
      MStatus l_status{};
      if (auto l_sum = std::reduce(l_face_vertex_counts.begin(), l_face_vertex_counts.end());
          l_sum != static_cast<int>(l_face_vertex_indices.length()))
        return MGlobal::displayError(
                   conv::to_ms(
                       fmt::format(
                           "面顶点数量与顶点索引数量不匹配, 面顶点数量总和: {}, 顶点索引数量: {}", l_sum,
                           l_face_vertex_indices.length()
                       )
                   )
               ),
               MS::kFailure;
      // l_u_array, l_v_array,
      l_fn_mesh.create(
          static_cast<unsigned int>(l_vertex_count), static_cast<unsigned int>(l_face_count), l_vertices,
          l_face_vertex_counts, l_face_vertex_indices, MObject::kNullObj, &l_status
      );
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    }
    return MS::kSuccess;
  }
};
dna_calib_import::dna_calib_import() : p_i(std::make_unique<impl>()) {}
dna_calib_import::~dna_calib_import() = default;

MStatus dna_calib_import::get_arg(const MArgList& in_arg) {
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_arg, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MSelectionList l_list{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_arg_data.getObjects(l_list));
  for (unsigned int i = 0; i < l_list.length(); ++i) {
    MObject l_node_obj{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_list.getDependNode(i, l_node_obj));
    if (!l_node_obj.hasFn(MFn::kDependencyNode)) continue;
    MFnDependencyNode l_fn_node{l_node_obj, &l_status};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if (l_fn_node.typeId() != dna_calib_node::doodle_id) continue;
    p_i->dna_node_obj = l_node_obj;
    break;
  }
  return MS::kSuccess;
}

MStatus dna_calib_import::doIt(const MArgList& in_list) {
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_arg(in_list));

  if (p_i->dna_node_obj.isNull())
    return displayError(conv::to_ms(fmt::format("未选择dna_calib_node节点"))), MS::kFailure;

  // 找到属性
  MStatus l_status{};
  MFnDependencyNode l_fn_node{p_i->dna_node_obj, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  auto l_plug_file_path = l_fn_node.findPlug(dna_calib_node::dna_file_path, false, &l_status);
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MString l_file_path_ms{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_plug_file_path.getValue(l_file_path_ms));
  p_i->file_path = conv::to_s(l_file_path_ms);
  if (p_i->file_path.empty() || !FSys::exists(p_i->file_path))
    return displayError(conv::to_ms(fmt::format("dna文件不存在: {}", p_i->file_path.generic_string()))), MS::kFailure;
  // 读取文件
  return p_i->import_dna_calib();
}

}  // namespace doodle::maya_plug