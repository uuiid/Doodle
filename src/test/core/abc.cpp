#include <doodle_core/doodle_core.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
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
#include <crtdbg.h>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <main_fixtures/lib_fixtures.h>
#include <string>
#include <vector>

using namespace std::literals;

BOOST_AUTO_TEST_CASE(maya_abc_r) {
  Alembic::Abc::IArchive l_ar{
      Alembic::AbcCoreOgawa::ReadArchive{},
      "D:/test_file/cloth_test/abc/cloth_test/cloth_test_cloth_test_rig_1001-1100.abc"s};

  Alembic::AbcGeom::IXform l_geo{l_ar.getTop(), "pCube1"s};
  Alembic::AbcGeom::IPolyMesh l_mesh{l_geo.getChild(0)};
  auto l_s       = l_mesh.getSchema();
  auto l_index_l = l_s.getUVsParam().getIndexedValue();

  BOOST_TEST(l_index_l.valid());

  {
    auto l_index_ll = *l_index_l.getIndices();
    for (auto i = 0; i < l_index_ll.size(); ++i) {
      BOOST_TEST_MESSAGE(i << " : " << l_index_ll[i]);
    }
  }

  {
    auto l_index_ll = *l_index_l.getVals();
    for (auto i = 0; i < l_index_ll.size(); ++i) {
      BOOST_TEST_MESSAGE(i << " : " << l_index_ll[i]);
    }
  }

  {
    std::vector<std::string> l_names;
    l_s.getFaceSetNames(l_names);
    for (auto& l_name : l_names) {
      BOOST_TEST_MESSAGE("face set name " << l_name);

      auto l_f   = l_s.getFaceSet(l_name);

      auto l_f_s = *l_f.getSchema().getValue().getFaces();
      for (auto i = 0; i < l_f_s.size(); ++i) {
        BOOST_TEST_MESSAGE(i << " : " << l_f_s[i]);
      }
    }
  }
}