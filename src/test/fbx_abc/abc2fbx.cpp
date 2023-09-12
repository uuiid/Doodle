//
// Created by td_main on 2023/9/12.
//

#include <doodle_app/app/program_options.h>

#include <doodle_lib/doodle_lib_all.h>

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

namespace doodle {

void run(const FSys::path& in_abc_path) {
  Alembic::Abc::IArchive l_archive{Alembic::AbcCoreOgawa::ReadArchive(), in_abc_path.string()};
  auto l_top = l_archive.getTop();
  if (l_top.getNumChildren() == 0) {
    DOODLE_LOG_ERROR("abc file is empty");
    return;
  }

  for (auto i = 0; i < l_top.getNumChildren(); ++i) {
    auto l_child = l_top.getChild(i);
    l_child.getHeader();
    if (Alembic::AbcGeom::IXform::matches(l_child.getHeader())) {
      auto l_xform = Alembic::AbcGeom::IXform{l_child};
      DOODLE_LOG_INFO("{}", l_xform.getName());
    }
  }
}

}  // namespace doodle

int http_proxy(int argc, char* argv[]) {
  using namespace doodle;
  argh::parser l_parser(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
  if (auto l_ss = l_parser("--abc"); l_ss) {
    FSys::path l_path{l_ss.str()};
    l_path = l_path.make_preferred();
    run(l_path);
  } else {
    DOODLE_LOG_ERROR("abc path is empty");
  }
  return 0;
}