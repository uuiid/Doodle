#include "AbcWrap/IAbcTransform.h"

namespace doodle
{
    IAbcTransform::IAbcTransform(Alembic::AbcGeom::IXform &InTransform,
                                 const FAbcFile *InFile,
                                 IAbcObject *InParent)
        : IAbcObject(InTransform, InFile, InParent),
          transform(InTransform),
          Schema(InTransform.getSchema())
    {
    }
}