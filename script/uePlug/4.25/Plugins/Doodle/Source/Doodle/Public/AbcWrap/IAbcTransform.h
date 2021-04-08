#pragma once

#include "IAbcObject.h"

namespace doodle
{

    class IAbcTransform : public IAbcObject
    {
        Alembic::AbcGeom::IXform transform;
        Alembic::AbcGeom::IXformSchema Schema;

    public:
        IAbcTransform(Alembic::AbcGeom::IXform &InTransform,
                      const FAbcFile *InFile,
                      IAbcObject *InParent);
        virtual ~IAbcTransform() override{};
    };

    
}