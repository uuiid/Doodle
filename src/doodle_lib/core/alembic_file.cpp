//
// Created by TD on 25-2-28.
//

#include "alembic_file.h"

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/IFaceSet.h>
#include <Alembic/AbcGeom/IGeomParam.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <Alembic/Util/Naming.h>
#include <Alembic/Util/PlainOldDataType.h>

namespace doodle::alembic {
std::vector<std::string> get_all_materials(const FSys::path& in_path) {
  std::vector<std::string> l_materials{};

  // 读取存档
  Alembic::Abc::IArchive l_archive{};
  Alembic::AbcCoreFactory::IFactory l_factory{};
  l_factory.setPolicy(Alembic::Abc::ErrorHandler::kThrowPolicy);
  l_archive  = l_factory.getArchive(in_path.string());

  // 迭代树
  auto l_top = l_archive.getTop();
  std::function<void(const Alembic::Abc::IObject&, std::vector<std::string>*)> l_iter{};
  l_iter = [&l_iter](const Alembic::Abc::IObject& in_object, std::vector<std::string>* in_materials) -> void {
    for (auto i = 0; i < in_object.getNumChildren(); ++i) {
      auto l_child = in_object.getChild(i);
      if (!l_child.valid()) continue;

      if (Alembic::AbcGeom::IPolyMesh::matches(l_child.getHeader())) {
        Alembic::AbcGeom::IPolyMesh l_mesh{l_child};
        auto& l_schema = l_mesh.getSchema();
        // 获取材质
        std::vector<std::string> l_mats{};
        l_schema.getFaceSetNames(l_mats);
        *in_materials |= ranges::actions::push_back(l_mats);
      }

      l_iter(l_child, in_materials);
    }
  };
  l_iter(l_top, &l_materials);

  return l_materials;
}
}  // namespace doodle::alembic