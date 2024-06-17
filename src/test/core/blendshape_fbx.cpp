
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/IArchive.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/IGeomBase.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/OFaceSet.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <Alembic/Util/PlainOldDataType.h>
#include <Eigen/Eigen>
// #include <Eigen/src/Core/Matrix.h>
// #include <Eigen/src/SVD/JacobiSVD.h>
#include <filesystem>
Eigen::MatrixXf load_abc_mesh(const std::filesystem::path& in_path) {
  Alembic::Abc::IArchive l_ar{Alembic::AbcCoreOgawa::ReadArchive{}, in_path.generic_string()};
  auto l_top        = l_ar.getTop();
  const auto l_meta = l_top.getMetaData();

  if (!Alembic::AbcGeom::IXform::matches(l_meta)) {
    doodle::default_logger_raw()->info("name {}", l_top.getName());
    const auto l_clild_count = l_top.getNumChildren();
    for (auto i = 0; i < l_clild_count; ++i) {
      auto l_child = l_top.getChild(i);
      doodle::default_logger_raw()->info("name {}", l_child.getName());
    }
  }
  return {};
}

BOOST_AUTO_TEST_CASE(blendshape_fbx) {
  auto l_mesh = load_abc_mesh("E:/Doodle/src/test/core/DBXY_EP360_SC001_AN_TianMeng_rig_HL_1001-1146.abc");
}