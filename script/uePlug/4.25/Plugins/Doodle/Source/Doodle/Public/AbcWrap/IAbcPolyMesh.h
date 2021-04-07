#pragma once

#include "IAbcObject.h"
#include "Components.h"

namespace doodle
{
    class IAbcPolyMesh : public IAbcObject
    {
    protected:
        //abc mesh
        Alembic::AbcGeom::IPolyMesh PolyMesh;
        //网格对象提取模式
        Alembic::AbcGeom::IPolyMeshSchema Schema;

        TArray<FString> materalName;
        void GetMaterial();

    public:
        IAbcPolyMesh(Alembic::AbcGeom::IPolyMesh &InPolyMesh,
                     const FAbcFile *InFile,
                     IAbcObject *InParent);
        virtual ~IAbcPolyMesh() override{};
    };

}