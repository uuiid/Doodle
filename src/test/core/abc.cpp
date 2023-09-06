#include <doodle_core/doodle_core.h>

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
#include <string>
#include <vector>

namespace fmt {}

using namespace std::literals;

auto print_abc_xform(Alembic::AbcGeom::IXform& in_xform) {
  Alembic::AbcGeom::IPolyMesh l_mesh{in_xform.getChild(0)};
  auto l_s       = l_mesh.getSchema();
  auto l_index_l = l_s.getUVsParam().getIndexedValue();

  BOOST_ASSERT(l_index_l.valid());

  {
    auto l_index_ll = *l_index_l.getIndices();
    for (auto i = 0; i < l_index_ll.size(); ++i) {
      DOODLE_LOG_INFO(" {} : {} ", i, l_index_ll[i]);
    }
  }

  {
    auto l_index_ll = *l_index_l.getVals();
    for (auto i = 0; i < l_index_ll.size(); ++i) {
      DOODLE_LOG_INFO(" {} : {} {} ", i, l_index_ll[i].x, l_index_ll[i].y);
    }
  }

  {
    std::vector<std::string> l_names;
    l_s.getFaceSetNames(l_names);
    for (auto& l_name : l_names) {
      DOODLE_LOG_INFO("face set name {} ", l_name);
      auto l_f   = l_s.getFaceSet(l_name);

      auto l_f_s = *l_f.getSchema().getValue().getFaces();
      for (auto i = 0; i < l_f_s.size(); ++i) {
        DOODLE_LOG_INFO(" {} : {} ", i, l_f_s[i]);
      }
    }
  }
}

auto maya_abc_r() {
  Alembic::Abc::IArchive l_ar{
      Alembic::AbcCoreOgawa::ReadArchive{},
      "D:/test_file/cloth_test/abc/cloth_test/cloth_test_cloth_test_rig_1001-1100.abc"s};
  //  Alembic::AbcGeom::IObject l_obj{l_ar, Alembic::Abc::kTop};
  auto l_top        = l_ar.getTop();
  const auto l_meta = l_top.getMetaData();
  if (!Alembic::AbcGeom::IXform::matches(l_meta)) {
    DOODLE_LOG_ERROR("not a xform, name {}", l_top.getName());

    const auto l_child_count = l_top.getNumChildren();
    for (auto i = 0; i < l_child_count; ++i) {
      auto l_child = l_top.getChild(i);
      DOODLE_LOG_INFO("child {} : {} ", i, l_child.getName());

      if (Alembic::AbcGeom::IXform::matches(l_child.getMetaData())) {
        DOODLE_LOG_INFO("child {} is a xform", i);
        Alembic::AbcGeom::IXform l_geo{l_child, Alembic::Abc::kWrapExisting};
        print_abc_xform(l_geo);
      }
    }
    return 0;
  }

  return 0;
}
int core_abc(int, char** const) { return maya_abc_r(); }