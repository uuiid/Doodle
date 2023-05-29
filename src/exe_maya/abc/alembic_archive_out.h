#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <Alembic/Abc/All.h>
#include <Alembic/Abc/OArchive.h>
#include <Alembic/Abc/OTypedScalarProperty.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/Util/All.h>
#include <cstdint>
#include <filesystem>
#include <maya/MApiNamespace.h>
#include <maya/MDagPath.h>
#include <memory>
#include <utility>
#include <vector>

namespace doodle::alembic {

class archive_out {
 public:
  using time_sampling_ptr    = Alembic::AbcCoreAbstract::TimeSamplingPtr;
  using o_archive_ptr        = std::shared_ptr<Alembic::Abc::OArchive>;
  using o_box3d_property_ptr = std::shared_ptr<Alembic::Abc::OBox3dProperty>;
  using o_xform_ptr          = std::shared_ptr<Alembic::AbcGeom::OXform>;
  using o_mesh_ptr           = std::shared_ptr<Alembic::AbcGeom::OPolyMesh>;

 private:
  struct dag_path_out_data {
    MDagPath dag_path_{};
    o_xform_ptr o_xform_ptr_{};
    o_mesh_ptr o_mesh_ptr_{};
  };
  std::string m_name{};
  MTime begin_time{};
  MTime end_time{};
  FSys::path out_path_{};

  o_archive_ptr o_archive_{};
  time_sampling_ptr shape_time_sampling_{};
  time_sampling_ptr transform_time_sampling_{};

  std::int32_t shape_time_index_{};
  std::int32_t transform_time_index_{};
  o_box3d_property_ptr o_box3d_property_ptr_{};
  std::vector<MDagPath> out_dag_path_{};
  std::vector<dag_path_out_data> dag_path_out_data_{};
  void open(const std::vector<MDagPath>& in_out_path);

  static std::tuple<std::uint16_t, std::uint16_t, std::uint16_t> get_rot_order();

  void wirte_transform(dag_path_out_data& in_path);
  void wirte_mesh(dag_path_out_data& in_path);
  void wirte_frame(const dag_path_out_data& in_path);
  void create_time_sampling_1();

 public:
  explicit archive_out(
      FSys::path in_path, time_sampling_ptr in_transform_time_sampling, time_sampling_ptr in_shape_time_sampling,
      const std::vector<MDagPath>& in_dag_path
  )
      : out_path_(std::move(in_path)),
        transform_time_sampling_(std::move(in_transform_time_sampling)),
        shape_time_sampling_(std::move(in_shape_time_sampling)) {
    open(in_dag_path);
  }

  explicit archive_out(FSys::path in_path, const std::vector<MDagPath>& in_dag_path) : out_path_(std::move(in_path)) {
    create_time_sampling_1();
    open(in_dag_path);
  }

  void write();
};

}  // namespace doodle::alembic