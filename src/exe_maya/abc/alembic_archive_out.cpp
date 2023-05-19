#include "alembic_archive_out.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/exception/exception.h>

#include "maya_plug/data/m_namespace.h"
#include "maya_plug/data/maya_conv_str.h"
#include "maya_plug/data/maya_tool.h"
#include "maya_plug/exception/exception.h"
#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_file_io.h>

#include "abc/alembic_archive_out.h"
#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <ImathVec.h>
#include <cmath>
#include <cstdint>
#include <fmt/core.h>
#include <maya/MApiNamespace.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MObject.h>
#include <maya/MQuaternion.h>
#include <maya/MStatus.h>
#include <maya/MVector.h>
#include <memory>
#include <range/v3/action/remove_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <utility>
#include <vector>

namespace doodle::alembic {

Alembic::AbcGeom::OXform archive_out::wirte_transform(const MDagPath& in_path) {
  MFnTransform l_fn_transform{in_path};

  auto l_name = maya_plug::m_namespace::strip_namespace_from_name(maya_plug::get_node_name(in_path));
  Alembic::AbcGeom::OXform l_oxform{*o_archive_, l_name, transform_time_index_};
  auto l_xform = l_oxform.getSchema();
  Alembic::AbcGeom::XformSample l_sample{};

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kTranslateHint};
    const auto l_translate = l_fn_transform.getTranslation(MSpace::Space::kWorld);
    l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
    l_sample.addOp(l_op);
  }

  {
    Alembic::AbcGeom::XformOp l_op{
        Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kRotatePivotTranslationHint};
    const auto l_translate = l_fn_transform.rotatePivotTranslation(MSpace::Space::kWorld);
    l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
    l_sample.addOp(l_op);
  }

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kRotateOperation, Alembic::AbcGeom::kRotateHint};
    MQuaternion l_rotate{};
    maya_plug::maya_chick(l_fn_transform.getRotation(l_rotate, MSpace::Space::kWorld));
    const auto l_rot = l_rotate.asEulerRotation().reorderIt(MEulerRotation::kXYZ);
    l_op.setXRotation(Alembic::AbcGeom::RadiansToDegrees(l_rot.x));
    l_op.setXRotation(Alembic::AbcGeom::RadiansToDegrees(l_rot.y));
    l_op.setXRotation(Alembic::AbcGeom::RadiansToDegrees(l_rot.z));
    l_sample.addOp(l_op);
  }
  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kRotatePivotPointHint};
    const auto l_translate = l_fn_transform.rotatePivot(MSpace::Space::kWorld);
    l_op.setTranslate({l_translate.x, l_translate.y, l_translate.z});
    l_sample.addOp(l_op);
  }

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kScaleOperation, Alembic::AbcGeom::kScaleHint};
    std::double_t l_scale[3]{};
    l_fn_transform.getScale(l_scale);

    l_op.setScale({l_scale[0], l_scale[1], l_scale[2]});
    l_sample.addOp(l_op);
  }

  {
    Alembic::AbcGeom::XformOp l_op{Alembic::AbcGeom::kTranslateOperation, Alembic::AbcGeom::kScalePivotPointHint};
    auto l_size = l_fn_transform.scalePivot(MSpace::Space::kWorld);
    l_op.setTranslate({l_size.x, l_size.y, l_size.z});
    l_sample.addOp(l_op);
  }

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
  return l_oxform;
}

void archive_out::wirte_mesh(const MDagPath& in_path) {
  MFnMesh l_mesh{in_path};
  MDagPath l_path_parent = in_path;
  l_path_parent.pop();
  auto l_tran = wirte_transform(l_path_parent);

  auto l_name = maya_plug::m_namespace::strip_namespace_from_name(maya_plug::get_node_name(in_path));

  // 这里是布料, 不使用细分网格
  Alembic::AbcGeom::OPolyMesh l_o_ploy_mesh{*o_archive_, l_name, shape_time_index_};
  auto l_ploy_mesh = l_o_ploy_mesh.getSchema();
  Alembic::AbcGeom::OV2fGeomParam l_uv_param{};

  // write uvs
  {
    MFloatArray l_u_array{};
    MFloatArray l_v_array{};
    {  // get uv set name
      auto l_uv_name = l_mesh.currentUVSetName();
      l_mesh.getUVs(l_u_array, l_v_array, &l_uv_name);
      if (l_u_array.length() != l_v_array.length()) {
        throw_exception(doodle_error{"{} uv length not equal", l_name});
      }
    }
    std::vector<Imath::V2f> l_uv_array{};
    l_uv_array.reserve(l_u_array.length());
    std::vector<std::uint32_t> l_index_array{};
    l_index_array.reserve(l_mesh.numFaceVertices());

    {
      for (auto i = 0; i < l_u_array.length(); ++i) {
        l_uv_array.emplace_back(l_u_array[i], l_v_array[i]);
      }
      std::int32_t l_uv_id{};
      for (auto i = 0; i < l_mesh.numPolygons(); ++i) {
        auto l_len = l_mesh.polygonVertexCount(i);

        for (auto j = 0; j < l_len; ++j) {
          l_mesh.getPolygonUVid(i, j, l_uv_id);
          l_index_array.emplace_back(l_uv_id);
        }
      }
    }

    // Alembic::AbcGeom::V2fArraySample l_uv_sample{l_uv_array.data(), l_uv_array.size()};

    auto l_uv_name = maya_plug::conv::to_s(l_mesh.currentUVSetName());
    Alembic::AbcGeom::OV2fGeomParam::Sample l_uv_sample{
        Alembic::Abc::V2fArraySample{l_uv_array.data(), l_uv_array.size()},
        Alembic::Abc::UInt32ArraySample{l_index_array.data(), l_index_array.size()},
        Alembic::AbcGeom::kFacevaryingScope};
  }
}

void archive_out::open() {
  o_archive_ = std::make_shared<Alembic::Abc::OArchive>(std::move(Alembic::Abc::v12::CreateArchiveWithInfo(
      Alembic::AbcCoreHDF5::WriteArchive{}, out_path_.generic_string(), "doodle alembic"s,
      maya_plug::maya_file_io::get_current_path().generic_string(), Alembic::Abc::ErrorHandler::kThrowPolicy
  )));

  if (o_archive_->valid()) {
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

  out_dag_path_ |= ranges::actions::remove_if([&](const MDagPath& in_dag) -> bool {
    return maya_plug::is_intermediate(in_dag) || !maya_plug::is_renderable(in_dag);
  });

  ranges::for_each(out_dag_path_, [&](const MDagPath& in_dag) {
    MStatus l_status{};
    if (in_dag.hasFn(MFn::kTransform)) {
      // MFnTransform transform{in_dag, &l_status};
      // if (l_status) {
      //   wirte_transform(in_dag);
      // }
    } else if (in_dag.hasFn(MFn::kMesh)) {
      wirte_mesh(in_dag);
    } else {
      DOODLE_LOG_WARN(
          "Does not export nodes other than transformation and grid nodes {} ", maya_plug::get_node_full_name(in_dag)
      );
    }
  });
}

void archive_out::write(const frame& in_frame) {}
}  // namespace doodle::alembic