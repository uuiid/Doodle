#include "AbcWrap/AbcFile.h"
#include "DoodleError.h"
#include "AbcWrap/IAbcPolyMesh.h"
#include "AbcWrap/IAbcTransform.h"

#include "Misc/Paths.h"
#include "AssetRegistryModule.h"
#include "Materials/MaterialInterface.h"
#include "Logging/TokenizedMessage.h"
#include "MeshUtilities.h"

namespace doodle
{

    void FAbcFile::TraverseAbcHierarchy(const Alembic::Abc::IObject &InObject,
                                        IAbcObject *InParent)
    {
        auto Header = InObject.getHeader();
        auto mate = Header.getMetaData();
        auto numChild = InObject.getNumChildren();

        auto abcobj = std::shared_ptr<IAbcObject>{nullptr};

        if (Alembic::AbcGeom::IPolyMesh::matches(Header))
        {
            auto mesh = Alembic::AbcGeom::IPolyMesh{InObject, Alembic::Abc::kWrapExisting};
            auto polyMesh = std::make_shared<IAbcPolyMesh>(mesh, this, InParent);
            abcobj = polyMesh;
        }
        else if (Alembic::AbcGeom::IXform::matches(Header))
        {
            auto XForm = Alembic::AbcGeom::IXform{InObject, Alembic::Abc::kWrapExisting};
            auto tran = std::make_shared<IAbcTransform>(XForm, this, InParent);

            abcobj = tran;
        }

        if (abcobj)
        {
            this->Objects.push_back(abcobj);
            MinTime = FMath::Min(MinTime, abcobj->GetTimeForFirstData());
            MaxTime = FMath::Max(MaxTime, abcobj->GetTimeForLastData());

            NumFrames = FMath::Max(NumFrames, abcobj->GetNumberOfSamples());
            MinFreameIndex = FMath::Max(MinFreameIndex, abcobj->GetFrameIndexForFirstData());
            MaxFreameIndex = FMath::Max(MaxFreameIndex, abcobj->GetFrameIndexForFirstData() +
                                                            abcobj->GetNumberOfSamples());
        }

        if (!this->RootObj && abcobj)
        {
            this->RootObj = abcobj.get();
        }

        for (uint32 ChildIndex = 0; ChildIndex < numChild; ++ChildIndex)
        {
            auto abcChildObj = InObject.getChild(ChildIndex);
            this->TraverseAbcHierarchy(abcChildObj, abcobj.get());
        }
    }

    FAbcFile::FAbcFile(const FString file)
        : Factory(),
          CompressionType(),
          Archive(),
          TopObject(),
          Objects(),
          PolyMeshes(),
          Transforms(),
          MinFreameIndex(),
          MaxFreameIndex(),
          NumFrames(),
          FPS(),
          FramesPerSecond(),
          MaterialMap(),
          ArchiveBounds(),
          MinTime(),
          MaxTime(),
          ImportLengthOffset(),
          ImportLength(),
          materalName(),
          FilePath(file),
          MeshUtilities()
    {
    }

    FAbcFile::~FAbcFile()
    {
    }

    bool FAbcFile::Open()
    {
        Factory.setPolicy(Alembic::Abc::ErrorHandler::kThrowPolicy);
        Factory.setOgawaNumStreams(12);
        FPaths::ConvertRelativePathToFull(FilePath);
        Archive = Factory.getArchive(TCHAR_TO_ANSI(*FPaths::ConvertRelativePathToFull(FilePath)), CompressionType);

        if (!Archive.valid())
            throw DoodleError{"abc"};
        //获得顶部obj
        Alembic::Abc::IObject{Archive, Alembic::Abc::kTop};
        TopObject = Archive.getTop();
        if (!TopObject.valid())
            throw DoodleError{"abc top not find"};
        this->TraverseAbcHierarchy(TopObject, nullptr);

        auto header = TopObject.getHeader();
        auto meta = header.getMetaData();
        auto boxProperty = Alembic::AbcGeom::GetIArchiveBounds(Archive, Alembic::Abc::ErrorHandler::kQuietNoopPolicy);

        if (boxProperty.valid())
        {
            ArchiveBounds = IAbcObject::ExtractBounds(boxProperty);
        }

        const auto TimeSamplingIndex = Archive.getNumTimeSamplings() > 1 ? 1 : 0;

        auto timeSampling = Archive.getTimeSampling(TimeSamplingIndex);
        if (timeSampling)
        {
            this->FPS = timeSampling->getTimeSamplingType().getTimePerCycle();
        }
        this->MeshUtilities = FModuleManager::Get().LoadModulePtr<IMeshUtilities>("MeshUtilities");

        return true;
    }

}