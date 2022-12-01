#include "alembic_file.h"

#include "doodle_core/exception/exception.h"

#include <Alembic/Abc/IArchive.h>
#include <Alembic/Abc/IObject.h>
#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <memory>


namespace doodle::dem_bones {

class alembic_file::impl {
 public:
  Alembic::Abc::IArchive archive;
  Alembic::AbcCoreFactory::IFactory factory;
  Alembic::Abc::IObject mesh_obj;
  Alembic::AbcGeom::IPolyMesh mesh;
  Alembic::AbcGeom::IPolyMeshSchema mesh_schema;
};
alembic_file::alembic_file() : p_i(std::make_unique<impl>()) {}

void alembic_file::open(const std::string& in_name) {
  // 抛出异常
  p_i->factory.setPolicy(Alembic::Abc::ErrorHandler::Policy::kThrowPolicy);

  p_i->archive = p_i->factory.getArchive(in_name);

  if (!p_i->archive.valid()) throw_exception(doodle_error{"无法打开存档"s});


  p_i->archive.getTop();
}

alembic_file::~alembic_file() = default;

}  // namespace doodle::dem_bones