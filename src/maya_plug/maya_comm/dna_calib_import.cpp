#include "dna_calib_import.h"

#include "doodle_core/exception/exception.h"

#include "maya_plug_fwd.h"
#include <maya_plug/node/dna_calib_node.h>

#include "data/maya_conv_str.h"
#include "data/maya_tool.h"
#include <arrayview/ArrayView.h>
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

  dnac::ScopedPtr<dnac::FileStream> dna_file_stream_;
  dnac::ScopedPtr<dnac::BinaryStreamReader> binary_stream_reader_;
  dnac::ScopedPtr<dnac::DNACalibDNAReader> dna_calib_dna_reader_;

  MStatus open_dna_file(const FSys::path& in_path) {
    file_path        = in_path;
    dna_file_stream_ = dnac::makeScoped<dnac::FileStream>(
        file_path.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
    );
    binary_stream_reader_ = dnac::makeScoped<dnac::BinaryStreamReader>(dna_file_stream_.get());
    binary_stream_reader_->read();
    if (!dnac::Status::isOk()) return display_error("读取dna文件失败: {} ", dnac::Status::get().message), MS::kFailure;

    dna_calib_dna_reader_ = dnac::makeScoped<dnac::DNACalibDNAReader>(binary_stream_reader_.get());
    return MS::kSuccess;
  }

  MStatus import_dna_calib() {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(open_dna_file(file_path));
    for (auto i = 0; i < dna_calib_dna_reader_->getMeshCount(); ++i) {
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_mesh_from_dna_mesh(i));
    }
    return MS::kSuccess;
  }
  MStatus create_mesh_from_dna_mesh(std::size_t in_mesh_idx) {
    auto l_name         = dna_calib_dna_reader_->getMeshName(in_mesh_idx);

    auto l_vertex_count = dna_calib_dna_reader_->getVertexPositionCount(in_mesh_idx);
    if (l_vertex_count == 0) return display_warning("Mesh {} 没有顶点, 跳过创建", l_name), MS::kSuccess;
    auto l_uv_count     = dna_calib_dna_reader_->getVertexTextureCoordinateCount(in_mesh_idx);
    auto l_layout_count = dna_calib_dna_reader_->getVertexLayoutCount(in_mesh_idx);
    auto l_face_count   = dna_calib_dna_reader_->getFaceCount(in_mesh_idx);
    display_info(
        "Mesh {}: 名称: {} 顶点数量: {} uv数量: {} layout数量: {}, 面数量: {}", in_mesh_idx, l_name, l_vertex_count,
        l_uv_count, l_layout_count, l_face_count
    );
    MPointArray l_vertices{};
    l_vertices.setLength(static_cast<unsigned int>(l_vertex_count));
    for (auto j = 0; j < l_vertex_count; ++j) {
      auto l_pos = dna_calib_dna_reader_->getVertexPosition(in_mesh_idx, j);
      l_vertices.set(static_cast<unsigned int>(j), l_pos.x, l_pos.y, l_pos.z);
    }
    std::vector<dna::VertexLayout> l_layouts{};
    l_layouts.reserve(static_cast<size_t>(l_layout_count));
    for (auto j = 0; j < l_layout_count; ++j) {
      auto l_layout = dna_calib_dna_reader_->getVertexLayout(in_mesh_idx, j);
      l_layouts.push_back(l_layout);
    }

    MIntArray l_face_vertex_counts{};
    MIntArray l_face_vertex_indices{};
    std::vector<dna::ConstArrayView<std::uint32_t>> l_face_layout_indices{};
    l_face_layout_indices.reserve(static_cast<size_t>(l_face_count));
    l_face_vertex_indices.setSizeIncrement(l_face_count);
    l_face_vertex_counts.setLength(static_cast<unsigned int>(l_face_count));
    for (auto j = 0; j < l_face_count; ++j) {
      auto l_indices = dna_calib_dna_reader_->getFaceVertexLayoutIndices(in_mesh_idx, j);
      l_face_layout_indices.push_back(l_indices);
      l_face_vertex_counts.set(static_cast<int>(l_indices.size()), static_cast<unsigned int>(j));
      for (auto k = 0; k < l_indices.size(); ++k) {
        l_face_vertex_indices.append(static_cast<int>(l_layouts[l_indices[k]].position));
      }
    }

    std::vector<dna::TextureCoordinate> l_uvs{};
    l_uvs.reserve(static_cast<size_t>(l_uv_count));
    for (auto j = 0; j < l_uv_count; ++j) {
      auto l_uv = dna_calib_dna_reader_->getVertexTextureCoordinate(in_mesh_idx, j);
      l_uvs.push_back(l_uv);
    }
    MFloatArray l_u_array{}, l_v_array{};
    MIntArray l_uv_indices{};
    l_u_array.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    l_v_array.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    l_uv_indices.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    std::size_t l_uv_idx_counter = 0;
    for (auto&& l_face : l_face_layout_indices) {
      for (auto&& l_idx : l_face) {
        // l_face_vertex_indices.append(static_cast<int>(l_layouts[l_idx].position));
        auto l_uv_idx = l_layouts[l_idx].textureCoordinate;
        if (l_uv_idx < l_uv_count) {
          l_u_array.append(l_uvs[l_uv_idx].u);
          l_v_array.append(l_uvs[l_uv_idx].v);
          l_uv_indices.append(static_cast<int>(l_uv_idx_counter));
          ++l_uv_idx_counter;
        } else {
          return display_warning(
                     "Mesh {} 面的顶点使用了不存在的uv索引: {}, 总uv数量: {}", l_name, l_uv_idx, l_uv_count
                 ),
                 MS::kFailure;
        }
      }
    }

    MFnMesh l_fn_mesh{};
    MStatus l_status{};
    if (auto l_sum = std::reduce(l_face_vertex_counts.begin(), l_face_vertex_counts.end());
        l_sum != static_cast<int>(l_face_vertex_indices.length()))
      return display_error(
                 "面顶点数量与顶点索引数量不匹配, 面顶点数量总和: {}, 顶点索引数量: {}", l_sum,
                 l_face_vertex_indices.length()
             ),
             MS::kFailure;
    // l_u_array, l_v_array,
    l_fn_mesh.create(
        static_cast<unsigned int>(l_vertex_count), static_cast<unsigned int>(l_face_count), l_vertices,
        l_face_vertex_counts, l_face_vertex_indices, MObject::kNullObj, &l_status
    );
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_mesh.setUVs(l_u_array, l_v_array));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_mesh.assignUVs(l_face_vertex_counts, l_uv_indices));
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

  if (p_i->dna_node_obj.isNull()) return display_error("未选择dna_calib_node节点"), MS::kFailure;

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
    return display_error("dna文件不存在: {}", p_i->file_path.generic_string()), MS::kFailure;
  // 读取文件
  return p_i->import_dna_calib();
}

}  // namespace doodle::maya_plug