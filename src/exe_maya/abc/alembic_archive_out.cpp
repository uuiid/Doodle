#include "alembic_archive_out.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/exception/exception.h>

#include "maya_plug/data/maya_tool.h"
#include <maya_plug/data/maya_file_io.h>

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcGeom/All.h>
#include <fmt/core.h>
#include <maya/MApiNamespace.h>
#include <maya/MDagPath.h>
#include <maya/MFnTransform.h>
#include <maya/MStatus.h>
#include <memory>
#include <range/v3/action/remove_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <utility>

namespace doodle::alembic {

void archive_out::wirte_transform(const MDagPath& in_path) {





  
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
      MFnTransform transform{in_dag, &l_status};
      if (l_status) {
        wirte_transform(in_dag);
      }
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