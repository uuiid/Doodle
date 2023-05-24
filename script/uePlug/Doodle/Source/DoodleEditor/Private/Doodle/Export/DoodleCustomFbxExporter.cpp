#include "DoodleCustomFbxExporter.h"
#include "CoreGlobals.h"

#include "Misc/FeedbackContext.h"
#include "FbxImporter.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/InstancedStaticMeshComponent.h"

class Doodle_CustomFbxExporter {
  // copy to FbxMainExport.cpp
  static void DetermineVertsToWeld(TArray<int32>& VertRemap, TArray<int32>& UniqueVerts, const FStaticMeshLODResources& RenderMesh) {
    const int32 VertexCount = RenderMesh.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();

    // Maps unreal verts to reduced list of verts
    VertRemap.Empty(VertexCount);
    VertRemap.AddUninitialized(VertexCount);

    // List of Unreal Verts to keep
    UniqueVerts.Empty(VertexCount);

    // Combine matching verts using hashed search to maintain good performance
    TMap<FVector, int32> HashedVerts;
    for (int32 a = 0; a < VertexCount; a++) {
      const FVector& PositionA = (FVector)RenderMesh.VertexBuffers.PositionVertexBuffer.VertexPosition(a);
      const int32* FoundIndex  = HashedVerts.Find(PositionA);
      if (!FoundIndex) {
        int32 NewIndex = UniqueVerts.Add(a);
        VertRemap[a]   = NewIndex;
        HashedVerts.Add(PositionA, NewIndex);
      } else {
        VertRemap[a] = *FoundIndex;
      }
    }
  }

  FbxSurfaceMaterial* CreateDefaultMaterial() {
    // TODO(sbc): the below cast is needed to avoid clang warning.  The upstream
    // signature in FBX should really use 'const char *'.
    FbxSurfaceMaterial* FbxMaterial = Scene->GetMaterial(const_cast<char*>("Fbx Default Material"));

    if (!FbxMaterial) {
      FbxMaterial = FbxSurfaceLambert::Create(Scene, "Fbx Default Material");
      ((FbxSurfaceLambert*)FbxMaterial)->Diffuse.Set(FbxDouble3(0.72, 0.72, 0.72));
    }

    return FbxMaterial;
  }

  FbxNode* FindActor(AActor* In_Actor) {
    if (FbxActors.Find(In_Actor)) {
      return *FbxActors.Find(In_Actor);
    } else {
      return nullptr;
    }
  }

 public:
  FbxManager* SdkManager;
  FbxScene* Scene;
  FbxAnimStack* AnimStack;
  FbxAnimLayer* AnimLayer;

  TSet<FString> UniqueActorNames;
  TMap<const AActor*, FName> ActorNames;

  TMap<const UStaticMesh*, FbxMesh*> FbxMeshes;
  TMap<const UMaterialInterface*, FbxSurfaceMaterial*> FbxMaterials;
  TMap<const AActor*, FbxNode*> FbxActors;
  UnFbx::FFbxDataConverter Converter{};

  Doodle_CustomFbxExporter() {
    SdkManager         = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(SdkManager, IOSROOT);
    SdkManager->SetIOSettings(ios);
    Scene                      = FbxScene::Create(SdkManager, "doodle_export");

    // create scene info
    FbxDocumentInfo* SceneInfo = FbxDocumentInfo::Create(SdkManager, "SceneInfo");
    SceneInfo->mTitle          = "Doodle FBX Exporter";
    SceneInfo->mSubject        = "Custom Export FBX meshes from Unreal";
    SceneInfo->Original_ApplicationVendor.Set("Doodle");
    SceneInfo->Original_ApplicationName.Set("Unreal Engine");
    SceneInfo->Original_ApplicationVersion.Set(TCHAR_TO_UTF8(*FEngineVersion::Current().ToString()));
    SceneInfo->LastSaved_ApplicationVendor.Set("Doodle");
    SceneInfo->LastSaved_ApplicationName.Set("Unreal Engine");
    SceneInfo->LastSaved_ApplicationVersion.Set(TCHAR_TO_UTF8(*FEngineVersion::Current().ToString()));

    Scene->SetSceneInfo(SceneInfo);

    // FbxScene->GetGlobalSettings().SetOriginalUpAxis(KFbxAxisSystem::Max);
    FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector)-FbxAxisSystem::eParityOdd;

    const FbxAxisSystem UnrealZUp(FbxAxisSystem::eZAxis, FrontVector, FbxAxisSystem::eRightHanded);
    Scene->GetGlobalSettings().SetAxisSystem(UnrealZUp);
    Scene->GetGlobalSettings().SetOriginalUpAxis(UnrealZUp);
    // Maya use cm by default
    Scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
    // FbxScene->GetGlobalSettings().SetOriginalSystemUnit( KFbxSystemUnit::m );

    Scene->GetGlobalSettings().SetTimeMode(FbxTime::eDefaultMode);

    // setup anim stack
    AnimStack = FbxAnimStack::Create(Scene, "Doodle Take");
    // KFbxSet<KTime>(AnimStack->LocalStart, KTIME_ONE_SECOND);
    AnimStack->Description.Set("Animation Take for Doodle Unreal.");

    // this take contains one base layer. In fact having at least one layer is mandatory.
    AnimLayer = FbxAnimLayer::Create(Scene, "Base Layer");
    AnimStack->AddMember(AnimLayer);
  }

  ~Doodle_CustomFbxExporter() {
    SdkManager->Destroy();
  }

  FString GetActorNodeName(const AActor* InActor) {
    if (ActorNames.Contains(InActor)) {
      return ActorNames[InActor].ToString();
    }

    FString ActorNodeName = InActor->GetActorLabel();

    if (UniqueActorNames.Contains(ActorNodeName)) {
      FString ActorNodeNamePrefix = ActorNodeName;
      int32 ActorNodeNameIndex    = 1;
      FActorLabelUtilities::SplitActorLabel(ActorNodeNamePrefix, ActorNodeNameIndex);

      do {
        ActorNodeName = FString::Printf(TEXT("%s%d"), *ActorNodeNamePrefix, ++ActorNodeNameIndex);
      } while (UniqueActorNames.Contains(ActorNodeName));
    }

    UniqueActorNames.Add(ActorNodeName);
    ActorNames.Add(InActor) = FName(ActorNodeName);

    return ActorNodeName;
  }

  void WriteToFile(const FString& In_FilePath) {
    // Create an exporter.
    FbxExporter* Exporter = FbxExporter::Create(SdkManager, "");
    int32 FileFormat      = SdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
    SdkManager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, false);

    if (!Exporter->SetFileExportVersion(FBX_2019_00_COMPATIBLE, FbxSceneRenamer::eNone)) {
      UE_LOG(LogTemp, Warning, TEXT("Call to KFbxExporter::SetFileExportVersion(FBX_2019_00_COMPATIBLE) to export 2019 fbx file format failed.\n"));
    }

    if (!Exporter->Initialize(TCHAR_TO_UTF8(*In_FilePath), FileFormat, SdkManager->GetIOSettings())) {
      UE_LOG(LogTemp, Warning, TEXT("Call to KFbxExporter::Initialize() failed.\n"));
      UE_LOG(LogTemp, Warning, TEXT("Error returned: %s\n\n"), Exporter->GetStatus().GetErrorString());
      return;
    }
    int32 Major, Minor, Revision;
    FbxManager::GetFileFormatVersion(Major, Minor, Revision);
    UE_LOG(LogTemp, Log, TEXT("FBX version number for this version of the FBX SDK is %d.%d.%d\n\n"), Major, Minor, Revision);

    // Export the scene.
    Exporter->Export(Scene);
    Exporter->Destroy();
  }

  FbxNode* ExportStaticMeshToFbx(const UStaticMesh* In_StaticMesh, const FString& In_MeshName, FbxNode* FbxActor) {
    FbxMesh* L_Mesh = FbxMeshes.FindRef(In_StaticMesh);

    if (!L_Mesh) {
      L_Mesh                                    = FbxMesh::Create(Scene, TCHAR_TO_UTF8(*In_MeshName));
      const FStaticMeshLODResources& RenderMesh = In_StaticMesh->GetLODForExport(0);

      // Verify the integrity of the static mesh.
      if (RenderMesh.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() == 0) {
        return nullptr;
      }
      if (RenderMesh.Sections.Num() == 0) {
        return nullptr;
      }
      // Remaps an Unreal vert to final reduced vertex list
      TArray<int32> VertRemap;
      TArray<int32> UniqueVerts;

      // Weld verts
      DetermineVertsToWeld(VertRemap, UniqueVerts, RenderMesh);

      // Create and fill in the vertex position data source.
      // The position vertices are duplicated, for some reason, retrieve only the first half vertices.
      const int32 VertexCount   = VertRemap.Num();
      const int32 PolygonsCount = RenderMesh.Sections.Num();

      L_Mesh->InitControlPoints(UniqueVerts.Num());

      FbxVector4* ControlPoints = L_Mesh->GetControlPoints();
      for (int32 PosIndex = 0; PosIndex < UniqueVerts.Num(); ++PosIndex) {
        int32 UnrealPosIndex    = UniqueVerts[PosIndex];
        FVector Position        = (FVector)RenderMesh.VertexBuffers.PositionVertexBuffer.VertexPosition(UnrealPosIndex);
        ControlPoints[PosIndex] = FbxVector4(Position.X, -Position.Y, Position.Z);
      }

      // Set the normals on Layer 0.
      FbxLayer* Layer = L_Mesh->GetLayer(0);
      if (Layer == nullptr) {
        L_Mesh->CreateLayer();
        Layer = L_Mesh->GetLayer(0);
      }

      // 建立多次重复使用的索引列表，以查询Normals、UV和其他每个面的顶点信息
      TArray<uint32> Indices;
      for (int32 PolygonsIndex = 0; PolygonsIndex < PolygonsCount; ++PolygonsIndex) {
        FIndexArrayView RawIndices         = RenderMesh.IndexBuffer.GetArrayView();
        const FStaticMeshSection& Polygons = RenderMesh.Sections[PolygonsIndex];
        const uint32 TriangleCount         = Polygons.NumTriangles;
        for (uint32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex) {
          for (uint32 PointIndex = 0; PointIndex < 3; PointIndex++) {
            uint32 UnrealVertIndex = RawIndices[Polygons.FirstIndex + ((TriangleIndex * 3) + PointIndex)];
            Indices.Add(UnrealVertIndex);
          }
        }
      }

      // Create and fill in the per-face-vertex normal data source.
      // We extract the Z-tangent and the X/Y-tangents which are also stored in the render mesh.
      FbxLayerElementNormal* LayerElementNormal     = FbxLayerElementNormal::Create(L_Mesh, "");
      FbxLayerElementTangent* LayerElementTangent   = FbxLayerElementTangent::Create(L_Mesh, "");
      FbxLayerElementBinormal* LayerElementBinormal = FbxLayerElementBinormal::Create(L_Mesh, "");

      // Set 3 NTBs per triangle instead of storing on positional control points
      LayerElementNormal->SetMappingMode(FbxLayerElement::eByPolygonVertex);
      LayerElementTangent->SetMappingMode(FbxLayerElement::eByPolygonVertex);
      LayerElementBinormal->SetMappingMode(FbxLayerElement::eByPolygonVertex);

      // Set the NTBs values for every polygon vertex.
      LayerElementNormal->SetReferenceMode(FbxLayerElement::eDirect);
      LayerElementTangent->SetReferenceMode(FbxLayerElement::eDirect);
      LayerElementBinormal->SetReferenceMode(FbxLayerElement::eDirect);

      TArray<FbxVector4> FbxNormals;
      TArray<FbxVector4> FbxTangents;
      TArray<FbxVector4> FbxBinormals;

      FbxNormals.AddUninitialized(VertexCount);
      FbxTangents.AddUninitialized(VertexCount);
      FbxBinormals.AddUninitialized(VertexCount);

      for (int32 NTBIndex = 0; NTBIndex < VertexCount; ++NTBIndex) {
        FVector3f Normal      = (FVector3f)(RenderMesh.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(NTBIndex));
        FbxVector4& FbxNormal = FbxNormals[NTBIndex];
        FbxNormal             = FbxVector4(Normal.X, -Normal.Y, Normal.Z);
        FbxNormal.Normalize();

        FVector3f Tangent      = (FVector3f)(RenderMesh.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(NTBIndex));
        FbxVector4& FbxTangent = FbxTangents[NTBIndex];
        FbxTangent             = FbxVector4(Tangent.X, -Tangent.Y, Tangent.Z);
        FbxTangent.Normalize();

        FVector3f Binormal      = -(FVector3f)(RenderMesh.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(NTBIndex));
        FbxVector4& FbxBinormal = FbxBinormals[NTBIndex];
        FbxBinormal             = FbxVector4(Binormal.X, -Binormal.Y, Binormal.Z);
        FbxBinormal.Normalize();
      }

      // Add one normal per each face index (3 per triangle)
      for (int32 FbxVertIndex = 0; FbxVertIndex < Indices.Num(); FbxVertIndex++) {
        uint32 UnrealVertIndex = Indices[FbxVertIndex];
        LayerElementNormal->GetDirectArray().Add(FbxNormals[UnrealVertIndex]);
        LayerElementTangent->GetDirectArray().Add(FbxTangents[UnrealVertIndex]);
        LayerElementBinormal->GetDirectArray().Add(FbxBinormals[UnrealVertIndex]);
      }

      Layer->SetNormals(LayerElementNormal);
      Layer->SetTangents(LayerElementTangent);
      Layer->SetBinormals(LayerElementBinormal);

      FbxNormals.Empty();
      FbxTangents.Empty();
      FbxBinormals.Empty();
      // 不去测试材质

      uint32 AccountedTriangles = 0;
      for (int32 PolygonsIndex = 0; PolygonsIndex < PolygonsCount; ++PolygonsIndex) {
        const FStaticMeshSection& Polygons = RenderMesh.Sections[PolygonsIndex];
        FIndexArrayView RawIndices         = RenderMesh.IndexBuffer.GetArrayView();

        UMaterialInterface* Material       = In_StaticMesh->GetMaterial(Polygons.MaterialIndex);
        // Static meshes contain one triangle list per element.
        // [GLAFORTE] Could it occasionally contain triangle strips? How do I know?
        uint32 TriangleCount               = Polygons.NumTriangles;

        int32 MatIndex                     = FbxActor->AddMaterial(CreateDefaultMaterial());

        // Determine the actual material index
        int32 ActualMatIndex               = MatIndex;
        // Copy over the index buffer into the FBX polygons set.
        for (uint32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex) {
          L_Mesh->BeginPolygon(ActualMatIndex);
          for (uint32 PointIndex = 0; PointIndex < 3; PointIndex++) {
            uint32 OriginalUnrealVertIndex = RawIndices[Polygons.FirstIndex + ((TriangleIndex * 3) + PointIndex)];
            int32 RemappedVertIndex        = VertRemap[OriginalUnrealVertIndex];
            L_Mesh->AddPolygon(RemappedVertIndex);
          }
          L_Mesh->EndPolygon();
        }

        AccountedTriangles += TriangleCount;
      }
      FbxMeshes.Emplace(In_StaticMesh, L_Mesh);
    } else {
      FbxActor->AddMaterial(CreateDefaultMaterial());
    }

    FbxActor->SetNodeAttribute(L_Mesh);
    return FbxActor;
  }

  void ExportInstancedMeshToFbx(const UInstancedStaticMeshComponent* InstancedMeshComp, const FString& MeshName, FbxNode* FbxActor) {
    const UStaticMesh* StaticMesh = InstancedMeshComp->GetStaticMesh();
    check(StaticMesh);

    const int32 NumInstances = InstancedMeshComp->GetInstanceCount();
    for (int32 InstanceIndex = 0; InstanceIndex < NumInstances; ++InstanceIndex) {
      FTransform RelativeTransform;
      if (ensure(InstancedMeshComp->GetInstanceTransform(InstanceIndex, RelativeTransform, /*bWorldSpace=*/false))) {
        FString L_Node_Name = StaticMesh->GetName().Replace(TEXT("-"), TEXT("_"));
        FbxNode* InstNode   = FbxNode::Create(Scene, TCHAR_TO_UTF8(*FString::Printf(TEXT("%s_%d"), *L_Node_Name, InstanceIndex)));

        InstNode->LclTranslation.Set(Converter.ConvertToFbxPos(RelativeTransform.GetTranslation()));
        InstNode->LclRotation.Set(Converter.ConvertToFbxRot(RelativeTransform.GetRotation().Euler()));
        InstNode->LclScaling.Set(Converter.ConvertToFbxScale(RelativeTransform.GetScale3D()));

        ExportStaticMeshToFbx(StaticMesh, L_Node_Name, InstNode);
        FbxActor->AddChild(InstNode);
      }
    }
  }

  auto CreateActorNode(AActor* In_Actor) {
    FString L_FbxNodeName = GetActorNodeName(In_Actor);

    FbxNode* ActorNode    = FindActor(In_Actor);
    FTransform L_Out{};
    if (ActorNode == nullptr) {
      ActorNode             = FbxNode::Create(Scene, TCHAR_TO_UTF8(*L_FbxNodeName));

      AActor* L_ParentActor = In_Actor->GetAttachParentActor();
      FbxNode* ParentNode   = FindActor(L_ParentActor);
      // get tran
      FVector ActorLocation, ActorRotation, ActorScale;
      FTransform RotationDirectionConvert{FTransform::Identity};

      if (L_ParentActor) {
        // 设置演员在变换上的默认位置       变换与FBX的Z - up不同：平移时反转Y轴，旋转时反转Y / Z角值。
        const FTransform RelativeTransform = RotationDirectionConvert * In_Actor->GetTransform().GetRelativeTransform(L_ParentActor->GetTransform());
        ActorLocation                      = RelativeTransform.GetTranslation();
        ActorRotation                      = RelativeTransform.GetRotation().Euler();
        ActorScale                         = RelativeTransform.GetScale3D();
        L_Out                              = RelativeTransform;
      } else {
        ParentNode = Scene->GetRootNode();
        if (ParentNode != NULL) {
          // In case the parent was not export, get the absolute transform
          const FTransform AbsoluteTransform = RotationDirectionConvert * In_Actor->GetTransform();
          ActorLocation                      = AbsoluteTransform.GetTranslation();
          ActorRotation                      = AbsoluteTransform.GetRotation().Euler();
          ActorScale                         = AbsoluteTransform.GetScale3D();
          L_Out                              = AbsoluteTransform;
        } else {
          const FTransform ConvertedTransform = RotationDirectionConvert * In_Actor->GetTransform();
          ActorLocation                       = ConvertedTransform.GetTranslation();
          ActorRotation                       = ConvertedTransform.GetRotation().Euler();
          ActorScale                          = ConvertedTransform.GetScale3D();
          L_Out                               = ConvertedTransform;
        }
      }

      ParentNode->AddChild(ActorNode);
      FbxActors.Add(In_Actor, ActorNode);

      ActorNode->LclTranslation.Set(Converter.ConvertToFbxPos(ActorLocation));
      ActorNode->LclRotation.Set(Converter.ConvertToFbxRot(ActorRotation));
      ActorNode->LclScaling.Set(Converter.ConvertToFbxScale(ActorScale));
    }
    return MakeTuple(ActorNode, L_Out);
  }

  void ExportActor(AActor* In_Actor) {
    auto [ActorNode, L_ActorNode_Tran] = CreateActorNode(In_Actor);

    // if (In_Actor->IsA(AStaticMeshActor::StaticClass())) {
    //   UStaticMesh* L_StaticMesh = CastChecked<AStaticMeshActor>(In_Actor)->GetStaticMeshComponent()->GetStaticMesh();
    //   FString L_FbxMeshName     = L_StaticMesh->GetName().Replace(TEXT("-"), TEXT("_"));

    //  ExportStaticMeshToFbx(L_StaticMesh, L_FbxMeshName, ActorNode);
    //} else {
    //}

    TArray L_Coms                      = GetExportComponent(In_Actor);

    for (auto&& Component : L_Coms) {
      FbxNode* ExportNode = ActorNode;
      if (L_Coms.Num() > 1) {
        // This actor has multiple components
        // create a child node under the actor for each component
        FbxNode* CompNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(*Component->GetName()));

        if (Component != In_Actor->GetRootComponent()) {
          // Transform is relative to the root component
          const FTransform RelativeTransform = FTransform::Identity * Component->GetComponentToWorld().GetRelativeTransform(In_Actor->GetTransform());
          CompNode->LclTranslation.Set(Converter.ConvertToFbxPos(RelativeTransform.GetTranslation()));
          CompNode->LclRotation.Set(Converter.ConvertToFbxRot(RelativeTransform.GetRotation().Euler()));
          CompNode->LclScaling.Set(Converter.ConvertToFbxScale(RelativeTransform.GetScale3D()));
        }

        ExportNode = CompNode;
        ActorNode->AddChild(CompNode);
      } else if (Component != In_Actor->GetRootComponent()) {
        // 在ActorNode变换中合并组件的相对变换，因为这是唯一要输出的组件，而且它不是根。
        const FTransform RelativeTransform = FTransform::Identity * Component->GetComponentToWorld().GetRelativeTransform(In_Actor->GetTransform());

        FTransform TotalTransform          = RelativeTransform * L_ActorNode_Tran;

        ActorNode->LclTranslation.Set(Converter.ConvertToFbxPos(TotalTransform.GetLocation()));
        ActorNode->LclRotation.Set(Converter.ConvertToFbxRot(TotalTransform.GetRotation().Euler()));
        ActorNode->LclScaling.Set(Converter.ConvertToFbxScale(TotalTransform.GetScale3D()));
      }

      UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component);

      if (StaticMeshComp && StaticMeshComp->GetStaticMesh()) {
        // if (USplineMeshComponent* SplineMeshComp = Cast<USplineMeshComponent>(StaticMeshComp)) {
        //   ExportSplineMeshToFbx(SplineMeshComp, *SplineMeshComp->GetName(), ExportNode);
        // } else
        if (UInstancedStaticMeshComponent* InstancedMeshComp = Cast<UInstancedStaticMeshComponent>(StaticMeshComp)) {
          ExportInstancedMeshToFbx(InstancedMeshComp, InstancedMeshComp->GetName(), ExportNode);
        } else {
          UStaticMesh* L_StaticMesh = StaticMeshComp->GetStaticMesh();
          FString L_FbxMeshName     = L_StaticMesh->GetName().Replace(TEXT("-"), TEXT("_"));

          ExportStaticMeshToFbx(L_StaticMesh, L_FbxMeshName, ActorNode);
        }
      }
    }
  }

  static TArray<USceneComponent*> GetExportComponent(const AActor* In_Actor) {
    TArray<USceneComponent*> L_Out{};
    TSet L_Coms = In_Actor->GetComponents();
    for (auto&& L_Com : In_Actor->GetComponents()) {
      if (auto L_Sub = Cast<UStaticMeshComponent>(L_Com)) {
        if (L_Sub->GetStaticMesh())
          L_Out.Emplace(L_Sub);
      }
    }
    return L_Out;
  }
};

UDoodleCustomFbxExporter::UDoodleCustomFbxExporter() : Impl_Data() {
  SupportedClass = UWorld::StaticClass();
  bText          = false;
  FormatExtension.Add(TEXT("dld_fbx"));
  FormatDescription.Add(TEXT("doodle custom export fbx"));
}

bool UDoodleCustomFbxExporter::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags) {
  UWorld* L_World = Cast<UWorld>(Object);

  if (!L_World) return false;
  // todo: 检查一下导出

  if (bSelectedOnly) {
    return false;
  }

  GWarn->BeginSlowTask(NSLOCTEXT("Doodle", "ExportingLevelToFBX", "Exporting Level To FBX"), true);
  ULevel* Level = L_World->PersistentLevel;

  {
    CreateDocument();

    GWarn->StatusUpdate(0, 1, NSLOCTEXT("UnrealEd", "ExportingLevelToFBX", "Exporting Level To FBX"));

    TArray<AActor*> L_Export_Actor{};

    for (TActorIterator<AActor> it{L_World}; it; ++it) {
      if (IsExport(*it)) {
        L_Export_Actor.Emplace(*it);
      }
    }

    SortActorsHierarchy(L_Export_Actor);

    for (auto&& i : L_Export_Actor) {
      Impl_Data->ExportActor(i);
    }

    WriteToFile(UExporter::CurrentFilename);
  }

  GWarn->EndSlowTask();

  return false;
}

void UDoodleCustomFbxExporter::CreateDocument() {
  Impl_Data = MakeShared<Doodle_CustomFbxExporter>();
}

void UDoodleCustomFbxExporter::WriteToFile(const FString& In_FilePath) {
  Impl_Data->WriteToFile(In_FilePath);
}

bool UDoodleCustomFbxExporter::IsExport(const AActor* In_Actor) {
  if (In_Actor->IsA(AStaticMeshActor::StaticClass()))
    return true;
  for (auto&& L_Com : In_Actor->GetComponents()) {
    if (auto L_Sub = Cast<UStaticMeshComponent>(L_Com)) {
      if (L_Sub->GetStaticMesh())
        return true;
    }
  }

  return false;
}

void UDoodleCustomFbxExporter::SortActorsHierarchy(TArray<AActor*>& In_Actors) {
  auto CalcAttachDepth = [](AActor& InActor) -> int32 {
    int32 Depth{0};
    if (InActor.GetRootComponent()) {
      for (const USceneComponent* Test = InActor.GetRootComponent()->GetAttachParent(); Test != nullptr; Test = Test->GetAttachParent(), Depth++)
        ;
    }

    return Depth;
  };
  // TArray.StableSort假设数组中没有空项
  In_Actors.StableSort([&](AActor& L, AActor& R) -> bool {
    return CalcAttachDepth(L) < CalcAttachDepth(R);
  });
  // 不幸的是，TArray.StableSort假设数组中没有空项，所以它迫使我使用内部的非限制性版本
  // StableSortInternal(Actors.GetData(), Actors.Num(), [&](AActor* L, AActor* R) {
  //  return CalcAttachDepth(L) < CalcAttachDepth(R);
  //});
}
