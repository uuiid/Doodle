#include "AbcWrap/IAbcPolyMesh.h"

namespace doodle
{
    IAbcPolyMesh::IAbcPolyMesh(Alembic::AbcGeom::IPolyMesh &InPolyMesh, const FAbcFile *InFile, IAbcObject *InParent)
        : IAbcObject(InPolyMesh, InFile, InParent),
          PolyMesh(InPolyMesh),
          Schema(InPolyMesh.getSchema())
    {
    }

    void IAbcPolyMesh::GetMaterial()
    {
        auto faceSetName = std::vector<std::string>{};
        Schema.getFaceSetNames(faceSetName);
        for (auto name : faceSetName)
        {
            materalName.Add(name.c_str());
        }
    }

}