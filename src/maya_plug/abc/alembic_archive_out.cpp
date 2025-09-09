#include "alembic_archive_out.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"

#include <boost/numeric/conversion/cast.hpp>

#include "maya_plug/data/m_namespace.h"
#include "maya_plug/data/maya_conv_str.h"
#include "maya_plug/data/maya_tool.h"
#include "maya_plug/exception/exception.h"
#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_file_io.h>

#include "abc/alembic_archive_out.h"
#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/OFaceSet.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <Alembic/Util/Naming.h>
#include <Alembic/Util/PlainOldDataType.h>
#include <ImathVec.h>
#include <cmath>
#include <cstdint>
#include <fmt/core.h>
#include <maya/MApiNamespace.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFn.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnTransform.h>
#include <maya/MIntArray.h>
#include <maya/MItDag.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MObject.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <memory>
#include <range/v3/action/remove_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <utility>
#include <vector>

namespace doodle::alembic {

// std::vector<float> get_normals() {}
namespace archive_out_ns {
/**
 * @brief 获取网格的uv
 * @param in_mesh 传入的网格
 * @return 返回uv的数组和索引数组
 * @warning Alembic::AbcGeom::OV2fGeomParam::Sample 测试中会发现移动构造有问题, 这里无法返回这个类型
 */
std::tuple<std::vector<Imath::V2f>, std::vector<std::uint32_t>> get_mesh_uv(const MFnMesh& in_mesh) {
  auto l_uv_name = in_mesh.currentUVSetName();
  auto l_uv_len  = in_mesh.numUVs(l_uv_name);

  std::vector<Imath::V2f> l_uv_array{};
  l_uv_array.reserve(l_uv_len);
  std::vector<std::uint32_t> l_index_array{};
  l_index_array.reserve(in_mesh.numFaceVertices());

  MFloatArray l_u_array{};
  MFloatArray l_v_array{};
  in_mesh.getUVs(l_u_array, l_v_array, &l_uv_name);
  if (l_u_array.length() != l_v_array.length()) {
    throw_exception(doodle_error{fmt::format("{} uv length not equal", maya_plug::get_node_name(in_mesh.object()))});
  }
  for (auto i = 0; i < l_u_array.length(); ++i) {
    l_uv_array.emplace_back(l_u_array[i], l_v_array[i]);
  }

  std::int32_t l_uv_id{};
  int faceCount = in_mesh.numPolygons();
  for (auto f = 0; f < faceCount; ++f) {
    auto l_len = in_mesh.polygonVertexCount(f);

    for (int i = l_len - 1; i >= 0; --i) {
      maya_plug::maya_chick(in_mesh.getPolygonUVid(f, i, l_uv_id, &l_uv_name));
      l_index_array.push_back(l_uv_id);
    }
  }

  // DOODLE_LOG_INFO("uv nums {} index {} ,{} ", l_uv_array.size(), l_index_array.size(), l_index_array);
  // DOODLE_LOG_INFO("uv maya nums {} index {}", in_mesh.numUVs(), in_mesh.numFaceVertices());
  // Alembic::AbcGeom::OV2fGeomParam::Sample l_uv_array_sample{};
  // l_uv_array_sample.setScope(Alembic::AbcGeom::kFacevaryingScope);
  // l_uv_array_sample.setVals(l_uv_array);
  // l_uv_array_sample.setIndices(l_index_array);

  return {l_uv_array, l_index_array};
}

std::vector<Imath::V3f> get_mesh_normals(const MFnMesh& in_mesh) {
  // MFloatVectorArray l_normal_array{};
  // l_mesh.getNormals(l_normal_array);
  std::vector<Imath::V3f> l_normal{};
  // l_normal.reserve(l_normal_array.length());
  // const auto l_flip_normals = maya_plug::get_plug(in_path.node(), "flipNormals").asBool();
  MIntArray l_poly_ver{};
  for (auto i = 0u; i < in_mesh.numPolygons(); ++i) {
    in_mesh.getPolygonVertices(i, l_poly_ver);
    // 在写进之前，重新打包这个向量中的法线顺序，以便Renderman也可以使用它
    int l_num_vertices = l_poly_ver.length();
    for (int v = l_num_vertices - 1; v >= 0; v--) {
      unsigned int vertexIndex = l_poly_ver[v];
      MVector normal;
      in_mesh.getFaceVertexNormal(i, vertexIndex, normal);

      // if (l_flip_normals) normal = -normal;

      l_normal.emplace_back(
          boost::numeric_cast<float>(normal[0]), boost::numeric_cast<float>(normal[1]),
          boost::numeric_cast<float>(normal[2])
      );
    }
  }
  // for (auto i = 0; i < l_normal_array.length(); ++i) {
  //   // if (l_flip_normals)
  //   //   l_normal.emplace_back(-l_normal_array[i].x, -l_normal_array[i].y, -l_normal_array[i].z);
  //   // else
  //   l_normal.emplace_back(l_normal_array[i].x, l_normal_array[i].y, l_normal_array[i].z);
  // }

  return l_normal;
}

std::tuple<std::vector<Alembic::Abc::V3f>, std::vector<Alembic::Util::int32_t>, std::vector<Alembic::Util::int32_t>>
get_mesh_poly(const MFnMesh& in_mesh) {
  std::vector<Alembic::Abc::V3f> l_point{};
  std::vector<Alembic::Util::int32_t> l_face_point{};
  std::vector<Alembic::Util::int32_t> l_point_counts{};
  MFloatPointArray l_m_point{};
  in_mesh.getPoints(l_m_point);
  if (l_m_point.length() < 3 && l_m_point.length() > 0) {
    MString err = in_mesh.fullPathName() + " is not a valid mesh, because it only has ";
    err += l_m_point.length();
    err += " points.";
    MGlobal::displayError(err);
    return {};
  }
  l_point.reserve(l_m_point.length());
  for (auto i = 0; i < l_m_point.length(); ++i) {
    l_point.emplace_back(l_m_point[i].x, l_m_point[i].y, l_m_point[i].z);
  }
  l_face_point.reserve(in_mesh.numPolygons());
  MIntArray l_m_face{};
  for (auto i = 0; i < in_mesh.numPolygons(); ++i) {
    in_mesh.getPolygonVertices(i, l_m_face);
    if (l_m_face.length() < 3) {
      MGlobal::displayWarning("Skipping degenerate polygon");
      continue;
    }
    int l_face_len = l_m_face.length() - 1;
    ///  倒着写，因为Maya中的多边形与Renderman的顺序不同（顺时针与逆时针？）
    for (auto j = l_face_len; j > -1; --j) l_face_point.push_back(l_m_face[j]);
    l_point_counts.push_back(l_m_face.length());
  }
  return {l_point, l_face_point, l_point_counts};
}
/**
 * @brief 获取网格的面集
 *
 * @param in_path 传入的着色集
 * @param in_obj_path 传入的对象路径
 * @return std::tuple<std::string, std::vector<std::int32_t>> 对应的材质名称, 面列表
 */
std::tuple<std::string, std::vector<std::int32_t>> get_face_set(const MObject& in_path, const MDagPath& in_obj_path) {
  MSelectionList l_select_list{};
  MObject l_com_obj{};
  MDagPath l_com_path{};
  MFnSet l_set{};
  maya_plug::maya_chick(l_set.setObject(in_path));
  auto l_mat      = maya_plug::details::shading_engine_to_mat(in_path);
  auto l_mat_name = maya_plug::m_namespace::strip_namespace_from_name(maya_plug::get_node_full_name(l_mat));
  l_set.getMembers(l_select_list, true);
  std::vector<Alembic::Util::int32_t> l_face_set_data{};
  for (MItSelectionList l_it{l_select_list}; !l_it.isDone(); l_it.next()) {
    if (l_it.hasComponents()) {
      l_it.getDagPath(l_com_path, l_com_obj);
      if (l_com_path == in_obj_path && !l_com_obj.isNull()) {
        MStatus l_s{};
        MIntArray l_indices{};
        MFnSingleIndexedComponent l_comp{l_com_obj, &l_s};
        maya_plug::maya_chick(l_s);
        maya_plug::maya_chick(l_comp.getElements(l_indices));

        l_face_set_data.reserve(l_indices.length());
        for (auto j = 0u; j < l_indices.length(); ++j) {
          l_face_set_data.emplace_back(l_indices[j]);
        }
        break;
      }
    } else {
      l_it.getDagPath(l_com_path);
      if (l_com_path == in_obj_path) {
        MFnMesh l_mesh{l_com_path};

        l_face_set_data = ranges::views::ints(0, l_mesh.numPolygons()) | ranges::to_vector;
      }
    }
  }

  return l_face_set_data.empty() ? std::tuple<std::string, std::vector<std::int32_t>>{}
                                 : std::tuple<std::string, std::vector<std::int32_t>>{l_mat_name, l_face_set_data};
}

MBoundingBox get_bound_box(const MDagPath& in_path) {
  MFnDagNode l_node{};
  MBoundingBox l_box_main{};
  auto l_path = in_path;
  MItDag it{};
  it.reset(l_path, MItDag::kDepthFirst);
  // for (; !it.isDone(); it.next()) {
  // it.getPath(l_path);
  if (l_path.hasFn(MFn::kMesh)) {
    if (maya_plug::is_intermediate(l_path)) return l_box_main;
    l_node.setObject(l_path);
    auto l_box = l_node.boundingBox();
    l_box_main.expand(l_box);
  }
  // }
  return l_box_main;
}
}  // namespace archive_out_ns

void archive_out::wirte_transform(dag_path_out_data& in_path, const o_xform_ptr& in_parent_xform) {
  MStatus l_s{};
  auto l_name = maya_plug::m_namespace::strip_namespace_from_name(maya_plug::get_node_name(in_path.dag_path_));
  default_logger_raw()->log(log_loc(), level::level_enum::info, "开始写入变换 {}", l_name);
  MFnTransform l_fn_transform{in_path.dag_path_, &l_s};
  maya_plug::maya_chick(l_s);

  in_path.o_xform_ptr_ = std::make_shared<Alembic::AbcGeom::OXform>(
      in_parent_xform ? in_parent_xform->getSchema().getObject() : o_archive_->getTop(), l_name, transform_time_index_
  );

  auto& l_xform = in_path.o_xform_ptr_->getSchema();
  Alembic::AbcGeom::XformSample l_sample{};

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kTranslateHint};
    const auto l_translate = l_fn_transform.getTranslation(MSpace::Space::kTransform);
    l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
    l_sample.addOp(l_op);
  }

  // {
  //   Alembic::AbcGeom::XformOp l_op{
  //       Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kRotatePivotTranslationHint};
  //   const auto l_translate = l_fn_transform.rotatePivotTranslation(MSpace::Space::kWorld);
  //   l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
  //   l_sample.addOp(l_op);
  // }

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kRotateOperation, Alembic::AbcGeom::kRotateHint};
    MQuaternion l_rotate{};
    maya_plug::maya_chick(l_fn_transform.getRotation(l_rotate, MSpace::Space::kWorld));
    const auto l_rot = l_rotate.asEulerRotation().reorderIt(MEulerRotation::kXYZ);
    l_op.setChannelValue(0, Alembic::AbcGeom::RadiansToDegrees(l_rot.x));
    l_op.setChannelValue(1, Alembic::AbcGeom::RadiansToDegrees(l_rot.y));
    l_op.setChannelValue(2, Alembic::AbcGeom::RadiansToDegrees(l_rot.z));
    l_sample.addOp(l_op);
  }
  // {
  //   Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kRotatePivotPointHint};
  //   const auto l_translate = l_fn_transform.rotatePivot(MSpace::Space::kWorld);
  //   l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
  //   l_sample.addOp(l_op);
  // }

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kScaleOperation, Alembic::AbcGeom::kScaleHint};
    std::double_t l_scale[3]{};
    l_fn_transform.getScale(l_scale);

    l_op.setScale({l_scale[0], l_scale[1], l_scale[2]});
    l_sample.addOp(l_op);
  }

  // {
  //   Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kScalePivotPointHint};
  //   auto l_size = l_fn_transform.scalePivot(MSpace::Space::kWorld);
  //   l_op.setTranslate({l_size.x, l_size.y, l_size.z});
  //   l_sample.addOp(l_op);
  // }

  l_xform.set(l_sample);

  // // 写入自定义属性
  // for (auto i = 0; l_fn_transform.attributeCount(); ++i) {
  //   MObject l_attr = l_fn_transform.attribute(i);
  //   MFnAttribute l_fn_attr{l_attr};
  //   MPlug l_plug = l_fn_transform.findPlug(l_attr, true);
  //   if (!l_fn_attr.isReadable() || l_plug.isNull()) {
  //     continue;
  //   }
  //   auto l_attr_name = maya_plug::conv::to_s(l_plug.partialName(false, false, false, false, false, false));
  // }
}

void archive_out::wirte_mesh(dag_path_out_data& in_path) {
  MDagPath l_path = in_path.dag_path_;
  l_path.extendToShape();
  MFnMesh l_mesh{l_path};
  DOODLE_LOG_INFO("开始写入网格体 {}", maya_plug::get_node_name(l_path));

  auto l_name         = maya_plug::m_namespace::strip_namespace_from_name(maya_plug::get_node_name(l_path));

  // 这里是布料, 不使用细分网格
  in_path.o_mesh_ptr_ = std::make_shared<Alembic::AbcGeom::OPolyMesh>(
      *in_path.o_xform_ptr_, l_name, Alembic::Abc::kFull, shape_time_index_, shape_time_sampling_
  );
  auto& l_ploy_schema = in_path.o_mesh_ptr_->getSchema();

  auto l_uv_name      = maya_plug::conv::to_s(l_mesh.currentUVSetName());
  l_ploy_schema.setUVSourceName(l_uv_name);

  auto [l_p, l_f, l_pc] = archive_out_ns::get_mesh_poly(l_mesh);
  auto [l_uv, l_uv_i]   = archive_out_ns::get_mesh_uv(l_mesh);

  Alembic::AbcGeom::OV2fGeomParam::Sample l_uv_s{l_uv, l_uv_i, Alembic::AbcGeom::kFacevaryingScope};
  /// 固定写法, 不可使用初始化函数构造
  Alembic::AbcGeom::ON3fGeomParam::Sample l_normal_s{};
  auto l_normal = archive_out_ns::get_mesh_normals(l_mesh);
  if (!l_normal.empty()) {
    l_normal_s.setScope(Alembic::AbcGeom::kFacevaryingScope);
    l_normal_s.setVals(l_normal);
  } else {
    DOODLE_LOG_WARN("物体 {} 没有法线", l_name);
  }

  Alembic::AbcGeom::OPolyMeshSchema::Sample l_poly_samp{l_p, l_f, l_pc, l_uv_s, l_normal_s};
  l_ploy_schema.set(l_poly_samp);
  // write face set
  for (auto&& l_obj : maya_plug::get_shading_engines(l_path)) {
    auto [l_name, l_list] = archive_out_ns::get_face_set(l_obj, l_path);
    if (l_name.empty()) continue;
    Alembic::AbcGeom::OFaceSet l_face_set =
        l_ploy_schema.hasFaceSet(l_name) ? l_ploy_schema.getFaceSet(l_name) : l_ploy_schema.createFaceSet(l_name);
    Alembic::AbcGeom::OFaceSetSchema l_face_set_schema = l_face_set.getSchema();
    l_face_set_schema.set(Alembic::AbcGeom::OFaceSetSchema::Sample{l_list});
    l_face_set_schema.setFaceExclusivity(Alembic::AbcGeom::kFaceSetExclusive);
  }
}

void archive_out::wirte_frame(const dag_path_out_data& in_path) {
  MFnMesh l_mesh{in_path.dag_path_};

  // 这里是布料, 不使用细分网格
  auto& l_ploy_schema   = in_path.o_mesh_ptr_->getSchema();

  auto [l_p, l_f, l_pc] = archive_out_ns::get_mesh_poly(l_mesh);
  auto [l_uv, l_uv_i]   = archive_out_ns::get_mesh_uv(l_mesh);
  // DOODLE_LOG_INFO("uvi {}", l_uv_i);
  Alembic::AbcGeom::OV2fGeomParam::Sample l_uv_s{l_uv, l_uv_i, Alembic::AbcGeom::kFacevaryingScope};
  Alembic::AbcGeom::ON3fGeomParam::Sample l_normal_s{};
  auto l_normal = archive_out_ns::get_mesh_normals(l_mesh);
  if (!l_normal.empty()) {
    l_normal_s.setScope(Alembic::AbcGeom::kFacevaryingScope);
    l_normal_s.setVals(l_normal);
  } else {
    DOODLE_LOG_WARN("物体 {} 没有法线", maya_plug::get_node_name(in_path.dag_path_));
  }
  Alembic::AbcGeom::OPolyMeshSchema::Sample l_poly_samp{l_p, l_f, l_pc, l_uv_s, l_normal_s};

  l_ploy_schema.set(l_poly_samp);
}

void archive_out::write_box() {
  MFnDagNode l_node{};
  MBoundingBox l_box_main{};
  for (auto&& i : root_) {
    l_box_main.expand(archive_out_ns::get_bound_box(i.dag_path_));
  }
  Alembic::Abc::Box3d l_box3d{
      {l_box_main.min().x, l_box_main.min().y, l_box_main.min().z},
      {l_box_main.max().x, l_box_main.max().y, l_box_main.max().z}
  };
  o_box3d_property_ptr_->set(l_box3d);
}

void archive_out::create_time_sampling_2(const MTime& in_time_begin, const MTime& in_time_end) {
  std::vector<std::double_t> l_times{};
  for (auto t = in_time_begin; t < in_time_end; ++t) {
    l_times.emplace_back(t.as(MTime::kSeconds));
  }

  // shape_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
  //     Alembic::AbcCoreAbstract::TimeSamplingType{
  //         boost::numeric_cast<std::uint32_t>(l_times.size()), in_time_begin.value() * maya_plug::details::spf()},
  //     l_times
  // );
  // transform_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
  //     Alembic::AbcCoreAbstract::TimeSamplingType{
  //         boost::numeric_cast<std::uint32_t>(l_times.size()), in_time_begin.value() * maya_plug::details::spf()},
  //     l_times
  // );
  DOODLE_LOG_INFO(
      "检查到帧率 {}({}), 开始时间 {}({})", maya_plug::details::fps(), maya_plug::details::spf(), in_time_begin.value(),
      in_time_end.as(MTime::kSeconds)
  );
  shape_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
      maya_plug::details::spf(), in_time_begin.as(MTime::kSeconds)
  );
  transform_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
      maya_plug::details::spf(), in_time_begin.as(MTime::kSeconds)
  );
}

void archive_out::open(const std::vector<MDagPath>& in_out_path) {
  o_archive_ = std::make_shared<Alembic::Abc::OArchive>(std::move(
      Alembic::Abc::v12::CreateArchiveWithInfo(
          Alembic::AbcCoreOgawa::WriteArchive{}, out_path_.generic_string(), "doodle alembic"s,
          maya_plug::maya_file_io::get_current_path().generic_string(), Alembic::Abc::ErrorHandler::kThrowPolicy
      )
  ));

  if (!o_archive_->valid()) {
    throw_exception(doodle_error{fmt::format("not open file {}", out_path_)});
  }

  shape_time_index_     = o_archive_->addTimeSampling(*shape_time_sampling_);
  transform_time_index_ = o_archive_->addTimeSampling(*transform_time_sampling_);

  o_box3d_property_ptr_ = std::make_shared<o_box3d_property_ptr::element_type>(
      std::move(Alembic::AbcGeom::CreateOArchiveBounds(*o_archive_, transform_time_index_))
  );

  if (!o_box3d_property_ptr_->valid()) {
    throw_exception(doodle_error{fmt::format("not open file {}", out_path_)});
  }

  auto l_dag_path_out_data_ = in_out_path | ranges::views::filter([](const MDagPath& in_dag) -> bool {
                                //  maya_plug::get_shape(in_dag.node()).hasFn(MFn::kMesh)
                                //! in_dag.hasFn(MFn::kMesh)
                                return !maya_plug::is_intermediate(in_dag) && in_dag.hasFn(MFn::kMesh)
                                    /* || !maya_plug::is_renderable(in_dag) */;
                              }) |
                              ranges::views::transform([](const MDagPath& in_dag) -> dag_path_out_data {
                                return {in_dag, {}, {}, maya_plug::get_node_name(in_dag), {}};
                              }) |
                              ranges::to_vector;

  root_ = {dag_path_out_data{}};
  for (auto&& i : l_dag_path_out_data_) {
    auto l_begin = root_.begin();
    for (std::int32_t j = i.dag_path_.length() - 1; j >= 0; --j) {
      MDagPath l_sub_path{i.dag_path_};
      l_sub_path.pop(j);

      if (auto l_tree_it = ranges::find_if(
              std::begin(l_begin), std::end(l_begin),
              [&](const dag_path_out_data& in_value) { return in_value.dag_path_ == l_sub_path; }
          );
          l_tree_it != std::end(l_begin)) {
        l_begin = l_tree_it;
      } else {
        if (l_sub_path.hasFn(MFn::kTransform))
          l_begin =
              root_.append_child(l_begin, dag_path_out_data{l_sub_path, {}, {}, maya_plug::get_node_name(l_sub_path)});
      }
    }
    l_begin->write_mesh_ = true;
  }
}

void archive_out::write() {
  static std::function<void(tree<dag_path_out_data>::iterator)> l_iter_write{};
  l_iter_write = [&](tree<dag_path_out_data>::iterator in_iter) {
    for (auto&& i = in_iter.begin(); i != in_iter.end(); ++i) {
      wirte_transform(*i, in_iter->o_xform_ptr_);
      if (i->write_mesh_) {
        wirte_mesh(*i);
      }
      l_iter_write(i);
    }
  };
  static std::function<void(tree<dag_path_out_data>::iterator)> l_iter_write_frame{};
  l_iter_write_frame = [&](tree<dag_path_out_data>::iterator in_iter) {
    for (auto&& i = in_iter.begin(); i != in_iter.end(); ++i) {
      if (i->write_mesh_) wirte_frame(*i);
      l_iter_write_frame(i);
    }
  };

  if (!init_) {
    l_iter_write(root_.begin());
    write_box();
    init_ = true;
  } else {
    l_iter_write_frame(root_.begin());
    write_box();
  }
}

}  // namespace doodle::alembic