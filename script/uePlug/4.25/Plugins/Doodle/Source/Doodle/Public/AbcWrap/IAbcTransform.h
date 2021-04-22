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

        //! 这里所有的方法都要创建
        /** Begin IAbcObject overrides */
        virtual bool ReadFirstFrame(const float InTime, const int32 FrameIndex) final { return {}; };
        virtual void SetFrameAndTime(const float InTime, const int32 FrameIndex, const EFrameReadFlags InFlags, const int32 TargetIndex = INDEX_NONE) final{};
        virtual FMatrix GetMatrix(const int32 FrameIndex) const final { return {}; };
        virtual bool HasConstantTransform() const final { return {}; };
        virtual void PurgeFrameData(const int32 FrameIndex) final{};
        /** End IAbcObject overrides */
    };

}