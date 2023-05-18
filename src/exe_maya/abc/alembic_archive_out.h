#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <Alembic/Abc/All.h>
#include <Alembic/Abc/OArchive.h>
#include <Alembic/Abc/OTypedScalarProperty.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
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

  struct frame {
    MTime frame_{};
  };

 private:
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
  void open();

  void write(const frame& in_frame);

  void wirte_transform(const MDagPath& in_path);
  void wirte_mesh(const MDagPath& in_path);

 public:
  explicit archive_out(
      FSys::path in_path, time_sampling_ptr in_transform_time_sampling, time_sampling_ptr in_shape_time_sampling,
      std::vector<MDagPath> in_dag_path
  )
      : out_path_(std::move(in_path)),
        transform_time_sampling_(std::move(in_transform_time_sampling)),
        shape_time_sampling_(std::move(in_shape_time_sampling)),
        out_dag_path_(std::move(in_dag_path)) {
    open();
  }

  archive_out& operator<<(const frame& in_path) { return *this; }
};

}  // namespace doodle::alembic