// Copyright Epic Games, Inc. All Rights Reserved.

#include "Doodle/Abc/DoodleAlembicImportFactory.h"

#include "AI/Navigation/NavCollisionBase.h"
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Doodle/Abc/AbcImportLogger.h"
#include "Doodle/Abc/AbcImporter.h"
#include "Doodle/Abc/DoodleAbcAssetImportData.h"
#include "Doodle/Abc/DoodleAbcImportSettings.h"
#include "DoodleAbcModule.h"
#include "Editor.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "GeometryCache.h"
#include "HAL/FileManager.h"
#include "ImportUtils/StaticMeshImportUtils.h"
#include "Interfaces/IMainFrameModule.h"
#include "Math/UnrealMathUtility.h"
#include "Subsystems/AssetEditorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoodleAlembicImportFactory)

#define LOCTEXT_NAMESPACE "DoodleAlembicImportFactory"

DEFINE_LOG_CATEGORY_STATIC(LogAlembic, Log, All);

UDoodleAbcImportFactory::UDoodleAbcImportFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  bCreateNew     = false;
  bEditAfterNew  = true;
  SupportedClass = nullptr;

  bEditorImport  = true;
  bText          = false;

  // DefaultImportPriority = 0;
  ImportPriority = 0;
  // this->ImportPriority;

  Formats.Add(TEXT("abc;Alembic"));
}

void UDoodleAbcImportFactory::PostInitProperties() { Super::PostInitProperties(); }

FText UDoodleAbcImportFactory::GetDisplayName() const { return LOCTEXT("AlembicImportFactoryDescription", "Alembic"); }

bool UDoodleAbcImportFactory::DoesSupportClass(UClass* Class) {
  return (
      Class == UGeometryCache::StaticClass() || Class == USkeletalMesh::StaticClass() ||
      Class == UAnimSequence::StaticClass()
  );
}

UClass* UDoodleAbcImportFactory::ResolveSupportedClass() { return UStaticMesh::StaticClass(); }

bool UDoodleAbcImportFactory::FactoryCanImport(const FString& Filename) {
  const FString Extension = FPaths::GetExtension(Filename);
  return FPaths::GetExtension(Filename) == TEXT("abc");
}

UObject* UDoodleAbcImportFactory::FactoryCreateFile(
    UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms,
    FFeedbackContext* Warn, bool& bOutOperationCanceled
) {
  const bool bIsUnattended =
      (IsAutomatedImport() || FApp::IsUnattended() || IsRunningCommandlet() || GIsRunningUnattendedScript);

  // Check if it's a re-import
  if (InParent != nullptr) {
    UObject* ExistingObject = StaticFindObject(UObject::StaticClass(), InParent, *InName.ToString());
    if (ExistingObject) {
      // Use this class as no other re-import handler exist for Alembics, yet
      FReimportHandler* ReimportHandler = this;
      TArray<FString> Filenames;
      Filenames.Add(UFactory::CurrentFilename);
      // Set the new source path before starting the re-import
      FReimportManager::Instance()->UpdateReimportPaths(ExistingObject, Filenames);
      // Do the re-import and exit
      const bool bIsAutomated            = bIsUnattended;
      const bool bShowNotification       = !bIsAutomated;
      const bool bAskForNewFileIfMissing = true;
      const FString PreferredReimportFile;
      const int32 SourceFileIndex = INDEX_NONE;
      const bool bForceNewFile    = false;
      FReimportManager::Instance()->Reimport(
          ExistingObject, bAskForNewFileIfMissing, bShowNotification, PreferredReimportFile, ReimportHandler,
          SourceFileIndex, bForceNewFile, bIsAutomated
      );
      return ExistingObject;
    }
  }

  GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(
      this, InClass, InParent, InName, TEXT("ABC")
  );

  // Use (and show) the settings from the script if provided
  UDoodleAbcImportSettings* ScriptedSettings =
      AssetImportTask ? Cast<UDoodleAbcImportSettings>(AssetImportTask->Options) : nullptr;
  if (ScriptedSettings) {
    ImportSettings = ScriptedSettings;
  }

  FAbcImporter Importer;
  EAbcImportError ErrorCode = Importer.OpenAbcFileForImport(Filename);
  ImportSettings->bReimport = false;
  AdditionalImportedObjects.Empty();

  // Set up message log page name to separate different assets
  FText ImportingText     = FText::Format(LOCTEXT("AbcFactoryImporting", "Importing {0}.abc"), FText::FromName(InName));
  const FString& PageName = ImportingText.ToString();

  if (ErrorCode != AbcImportError_NoError) {
    FAbcImportLogger::OutputMessages(PageName);

    // Failed to read the file info, fail the import
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
    return nullptr;
  }

  // Don't override the frame end value from the script except if it was unset
  if (ImportSettings->SamplingSettings.FrameEnd == 0) {
    ImportSettings->SamplingSettings.FrameEnd = Importer.GetEndFrameIndex();
  }

  bOutOperationCanceled = false;

  TArray<UObject*> ResultAssets;
  if (!bOutOperationCanceled) {
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(
        this, InClass, InParent, InName, TEXT("ABC")
    );

    int32 NumThreads = 1;
    if (FPlatformProcess::SupportsMultithreading()) {
      NumThreads = FPlatformMisc::NumberOfCores();
    }

    // Import file
    ErrorCode = Importer.ImportTrackData(NumThreads, ImportSettings);

    if (ErrorCode != AbcImportError_NoError) {
      // Failed to read the file info, fail the import
      GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
      FAbcImportLogger::OutputMessages(PageName);
      return nullptr;
    } else {
      if (ImportSettings->ImportType == EDoodleAlembicImportType::GeometryCache) {
        UObject* GeometryCache = ImportGeometryCache(Importer, InParent, Flags);
        if (GeometryCache) {
          ResultAssets.Add(GeometryCache);
        }
      } else if (ImportSettings->ImportType == EDoodleAlembicImportType::Skeletal) {
        TArray<UObject*> SkeletalMesh = ImportSkeletalMesh(Importer, InParent, Flags);
        ResultAssets.Append(SkeletalMesh);
      }
    }

    AdditionalImportedObjects.Reserve(ResultAssets.Num());
    for (UObject* Object : ResultAssets) {
      if (Object) {
        FAssetRegistryModule::AssetCreated(Object);
        GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Object);
        Object->MarkPackageDirty();
        Object->PostEditChange();
        AdditionalImportedObjects.Add(Object);
      }
    }

    FAbcImportLogger::OutputMessages(PageName);
  }

  return (ResultAssets.Num() > 0) ? ResultAssets[0] : nullptr;
}

UObject* UDoodleAbcImportFactory::ImportGeometryCache(FAbcImporter& Importer, UObject* InParent, EObjectFlags Flags) {
  // Flush commands before importing
  FlushRenderingCommands();

  const uint32 NumMeshes = Importer.GetNumMeshTracks();
  // Check if the alembic file contained any meshes
  if (NumMeshes > 0) {
    UGeometryCache* GeometryCache = Importer.ImportAsGeometryCache(InParent, Flags);

    if (!GeometryCache) {
      return nullptr;
    }

    // Setup asset import data
    if (!GeometryCache->AssetImportData || !GeometryCache->AssetImportData->IsA<UDoodleAbcAssetImportData>()) {
      GeometryCache->AssetImportData = NewObject<UDoodleAbcAssetImportData>(GeometryCache);
    }
    GeometryCache->AssetImportData->Update(UFactory::CurrentFilename);
    UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(GeometryCache->AssetImportData);
    if (AssetImportData) {
      Importer.UpdateAssetImportData(AssetImportData);
    }

    return GeometryCache;
  } else {
    // Not able to import a static mesh
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
    return nullptr;
  }
}

TArray<UObject*> UDoodleAbcImportFactory::ImportSkeletalMesh(
    FAbcImporter& Importer, UObject* InParent, EObjectFlags Flags
) {
  // Flush commands before importing
  FlushRenderingCommands();

  const uint32 NumMeshes = Importer.GetNumMeshTracks();
  // Check if the alembic file contained any meshes
  if (NumMeshes > 0) {
    TArray<UObject*> GeneratedObjects = Importer.ImportAsSkeletalMesh(InParent, Flags);

    if (!GeneratedObjects.Num()) {
      return {};
    }

    USkeletalMesh* SkeletalMesh = [&GeneratedObjects]() {
      UObject** FoundObject =
          GeneratedObjects.FindByPredicate([](UObject* Object) { return Object->IsA<USkeletalMesh>(); });
      return FoundObject ? CastChecked<USkeletalMesh>(*FoundObject) : nullptr;
    }();

    if (SkeletalMesh) {
      // Setup asset import data
      if (!SkeletalMesh->GetAssetImportData() ||
          !SkeletalMesh->GetAssetImportData()->IsA<UDoodleAbcAssetImportData>()) {
        SkeletalMesh->SetAssetImportData(NewObject<UDoodleAbcAssetImportData>(SkeletalMesh));
      }
      SkeletalMesh->GetAssetImportData()->Update(UFactory::CurrentFilename);
      UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(SkeletalMesh->GetAssetImportData());
      if (AssetImportData) {
        Importer.UpdateAssetImportData(AssetImportData);
      }
    }

    UAnimSequence* AnimSequence = [&GeneratedObjects]() {
      UObject** FoundObject =
          GeneratedObjects.FindByPredicate([](UObject* Object) { return Object->IsA<UAnimSequence>(); });
      return FoundObject ? CastChecked<UAnimSequence>(*FoundObject) : nullptr;
    }();

    if (AnimSequence) {
      // Setup asset import data
      if (!AnimSequence->AssetImportData || !AnimSequence->AssetImportData->IsA<UDoodleAbcAssetImportData>()) {
        AnimSequence->AssetImportData = NewObject<UDoodleAbcAssetImportData>(AnimSequence);
      }
      AnimSequence->AssetImportData->Update(UFactory::CurrentFilename);
      UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(AnimSequence->AssetImportData);
      if (AssetImportData) {
        Importer.UpdateAssetImportData(AssetImportData);
      }
    }

    return GeneratedObjects;
  } else {
    // Not able to import as skeletal mesh
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
    return {};
  }
}

bool UDoodleAbcImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames) {
  UAssetImportData* ImportData = nullptr;
  if (Obj->GetClass() == UGeometryCache::StaticClass()) {
    UGeometryCache* Cache = Cast<UGeometryCache>(Obj);
    ImportData            = Cache->AssetImportData;
  } else if (Obj->GetClass() == USkeletalMesh::StaticClass()) {
    USkeletalMesh* Cache = Cast<USkeletalMesh>(Obj);
    ImportData           = Cache->GetAssetImportData();
  } else if (Obj->GetClass() == UAnimSequence::StaticClass()) {
    UAnimSequence* Cache = Cast<UAnimSequence>(Obj);
    ImportData           = Cache->AssetImportData;
  }

  if (ImportData) {
    if (FPaths::GetExtension(ImportData->GetFirstFilename()) == TEXT("abc") ||
        (Obj->GetClass() == UAnimSequence::StaticClass() && ImportData->GetFirstFilename().IsEmpty())) {
      ImportData->ExtractFilenames(OutFilenames);
      return true;
    }
  }
  return false;
}

void UDoodleAbcImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) {
  USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
  if (SkeletalMesh && SkeletalMesh->GetAssetImportData() && ensure(NewReimportPaths.Num() == 1)) {
    SkeletalMesh->GetAssetImportData()->UpdateFilenameOnly(NewReimportPaths[0]);
  }

  UAnimSequence* Sequence = Cast<UAnimSequence>(Obj);
  if (Sequence && Sequence->AssetImportData && ensure(NewReimportPaths.Num() == 1)) {
    Sequence->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
  }

  UGeometryCache* GeometryCache = Cast<UGeometryCache>(Obj);
  if (GeometryCache && GeometryCache->AssetImportData && ensure(NewReimportPaths.Num() == 1)) {
    GeometryCache->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
  }
}

EReimportResult::Type UDoodleAbcImportFactory::Reimport(UObject* Obj) {
  ImportSettings->bReimport = true;

  const bool bIsUnattended =
      (IsAutomatedImport() || FApp::IsUnattended() || IsRunningCommandlet() || GIsRunningUnattendedScript);

  FText ReimportingText =
      FText::Format(LOCTEXT("AbcFactoryReimporting", "Reimporting {0}.abc"), FText::FromString(Obj->GetName()));
  const FString& PageName = ReimportingText.ToString();
  if (Obj->GetClass() == USkeletalMesh::StaticClass()) {
    USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
    if (!SkeletalMesh) {
      return EReimportResult::Failed;
    }

    CurrentFilename              = SkeletalMesh->GetAssetImportData()->GetFirstFilename();

    EReimportResult::Type Result = ReimportSkeletalMesh(SkeletalMesh);

    if (SkeletalMesh->GetOuter()) {
      SkeletalMesh->GetOuter()->MarkPackageDirty();
    } else {
      SkeletalMesh->MarkPackageDirty();
    }

    // Close possible open editors using this asset
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(SkeletalMesh);

    FAbcImportLogger::OutputMessages(PageName);
    return Result;
  } else if (Obj->GetClass() == UAnimSequence::StaticClass()) {
    UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj);
    if (!AnimSequence) {
      return EReimportResult::Failed;
    }

    CurrentFilename             = AnimSequence->AssetImportData->GetFirstFilename();
    USkeletalMesh* SkeletalMesh = nullptr;
    for (TObjectIterator<USkeletalMesh> It; It; ++It) {
      // This works because the skeleton is unique for every imported alembic cache
      if (It->GetSkeleton() == AnimSequence->GetSkeleton()) {
        SkeletalMesh = *It;
        break;
      }
    }

    if (!SkeletalMesh) {
      return EReimportResult::Failed;
    }

    // Close possible open editors using this asset
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(AnimSequence);

    EReimportResult::Type Result = ReimportSkeletalMesh(SkeletalMesh);

    if (SkeletalMesh->GetOuter()) {
      SkeletalMesh->GetOuter()->MarkPackageDirty();
    } else {
      SkeletalMesh->MarkPackageDirty();
    }

    FAbcImportLogger::OutputMessages(PageName);
    return Result;
  }

  return EReimportResult::Failed;
}

EReimportResult::Type UDoodleAbcImportFactory::ReimportGeometryCache(UGeometryCache* Cache) {
  // Ensure that the file provided by the path exists
  if (IFileManager::Get().FileSize(*CurrentFilename) == INDEX_NONE) {
    return EReimportResult::Failed;
  }

  FAbcImporter Importer;
  EAbcImportError ErrorCode = Importer.OpenAbcFileForImport(CurrentFilename);

  if (ErrorCode != AbcImportError_NoError) {
    // Failed to read the file info, fail the re importing process
    return EReimportResult::Failed;
  }

  ImportSettings->ImportType                  = EDoodleAlembicImportType::GeometryCache;
  ImportSettings->SamplingSettings.FrameStart = 0;
  ImportSettings->SamplingSettings.FrameEnd   = Importer.GetEndFrameIndex();

  if (Cache->AssetImportData && Cache->AssetImportData->IsA<UDoodleAbcAssetImportData>()) {
    UDoodleAbcAssetImportData* ImportData = Cast<UDoodleAbcAssetImportData>(Cache->AssetImportData);
    PopulateOptionsWithImportData(ImportData);
    Importer.RetrieveAssetImportData(ImportData);
  }
  {
    UDoodleAbcImportSettings* ScriptedSettings =
        AssetImportTask ? Cast<UDoodleAbcImportSettings>(AssetImportTask->Options) : nullptr;
    if (ScriptedSettings) {
      ImportSettings = ScriptedSettings;
    }
  }

  int32 NumThreads = 1;
  if (FPlatformProcess::SupportsMultithreading()) {
    NumThreads = FPlatformMisc::NumberOfCores();
  }

  // Import file
  ErrorCode = Importer.ImportTrackData(NumThreads, ImportSettings);

  if (ErrorCode != AbcImportError_NoError) {
    // Failed to read the file info, fail the re importing process
    return EReimportResult::Failed;
  }

  UGeometryCache* GeometryCache = Importer.ReimportAsGeometryCache(Cache);

  if (!GeometryCache) {
    return EReimportResult::Failed;
  } else {
    // Update file path/timestamp (Path could change if user has to browse for it manually)
    if (!GeometryCache->AssetImportData || !GeometryCache->AssetImportData->IsA<UDoodleAbcAssetImportData>()) {
      GeometryCache->AssetImportData = NewObject<UDoodleAbcAssetImportData>(GeometryCache);
    }

    GeometryCache->AssetImportData->Update(CurrentFilename);
    UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(GeometryCache->AssetImportData);
    if (AssetImportData) {
      Importer.UpdateAssetImportData(AssetImportData);
    }
  }

  return EReimportResult::Succeeded;
}

EReimportResult::Type UDoodleAbcImportFactory::ReimportSkeletalMesh(USkeletalMesh* SkeletalMesh) {
  // Ensure that the file provided by the path exists
  if (IFileManager::Get().FileSize(*CurrentFilename) == INDEX_NONE) {
    return EReimportResult::Failed;
  }

  FAbcImporter Importer;
  EAbcImportError ErrorCode = Importer.OpenAbcFileForImport(CurrentFilename);

  if (ErrorCode != AbcImportError_NoError) {
    // Failed to read the file info, fail the re importing process
    return EReimportResult::Failed;
  }

  if (SkeletalMesh->GetAssetImportData() && SkeletalMesh->GetAssetImportData()->IsA<UDoodleAbcAssetImportData>()) {
    UDoodleAbcAssetImportData* ImportData = Cast<UDoodleAbcAssetImportData>(SkeletalMesh->GetAssetImportData());
    PopulateOptionsWithImportData(ImportData);
    Importer.RetrieveAssetImportData(ImportData);
  }

  ImportSettings->ImportType                  = EDoodleAlembicImportType::Skeletal;
  ImportSettings->SamplingSettings.FrameStart = 0;
  ImportSettings->SamplingSettings.FrameEnd   = Importer.GetEndFrameIndex();

  {
    UDoodleAbcImportSettings* ScriptedSettings =
        AssetImportTask ? Cast<UDoodleAbcImportSettings>(AssetImportTask->Options) : nullptr;
    if (ScriptedSettings) {
      ImportSettings = ScriptedSettings;
    }
  }

  int32 NumThreads = 1;
  if (FPlatformProcess::SupportsMultithreading()) {
    NumThreads = FPlatformMisc::NumberOfCores();
  }

  // Import file
  ErrorCode = Importer.ImportTrackData(NumThreads, ImportSettings);

  if (ErrorCode != AbcImportError_NoError) {
    // Failed to read the file info, fail the re importing process
    return EReimportResult::Failed;
  }

  TArray<UObject*> ReimportedObjects = Importer.ReimportAsSkeletalMesh(SkeletalMesh);
  USkeletalMesh* NewSkeletalMesh     = [&ReimportedObjects]() {
    UObject** FoundObject =
        ReimportedObjects.FindByPredicate([](UObject* Object) { return Object->IsA<USkeletalMesh>(); });
    return FoundObject ? CastChecked<USkeletalMesh>(*FoundObject) : nullptr;
  }();

  if (!NewSkeletalMesh) {
    return EReimportResult::Failed;
  } else {
    // Update file path/timestamp (Path could change if user has to browse for it manually)
    if (!NewSkeletalMesh->GetAssetImportData() ||
        !NewSkeletalMesh->GetAssetImportData()->IsA<UDoodleAbcAssetImportData>()) {
      NewSkeletalMesh->SetAssetImportData(NewObject<UDoodleAbcAssetImportData>(NewSkeletalMesh));
    }

    NewSkeletalMesh->GetAssetImportData()->Update(CurrentFilename);
    UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(NewSkeletalMesh->GetAssetImportData());
    if (AssetImportData) {
      Importer.UpdateAssetImportData(AssetImportData);
    }
  }

  UAnimSequence* NewAnimSequence = [&ReimportedObjects]() {
    UObject** FoundObject =
        ReimportedObjects.FindByPredicate([](UObject* Object) { return Object->IsA<UAnimSequence>(); });
    return FoundObject ? CastChecked<UAnimSequence>(*FoundObject) : nullptr;
  }();

  if (!NewAnimSequence) {
    return EReimportResult::Failed;
  } else {
    // Update file path/timestamp (Path could change if user has to browse for it manually)
    if (!NewAnimSequence->AssetImportData || !NewAnimSequence->AssetImportData->IsA<UDoodleAbcAssetImportData>()) {
      NewAnimSequence->AssetImportData = NewObject<UDoodleAbcAssetImportData>(NewAnimSequence);
    }

    NewAnimSequence->AssetImportData->Update(CurrentFilename);
    UDoodleAbcAssetImportData* AssetImportData = Cast<UDoodleAbcAssetImportData>(NewAnimSequence->AssetImportData);
    if (AssetImportData) {
      Importer.UpdateAssetImportData(AssetImportData);
    }
  }

  return EReimportResult::Succeeded;
}

void UDoodleAbcImportFactory::PopulateOptionsWithImportData(UDoodleAbcAssetImportData* ImportData) {
  ImportSettings->NormalGenerationSettings = ImportData->NormalGenerationSettings;
  ImportSettings->CompressionSettings      = ImportData->CompressionSettings;
  ImportSettings->GeometryCacheSettings    = ImportData->GeometryCacheSettings;
  ImportSettings->ConversionSettings       = ImportData->ConversionSettings;
}

int32 UDoodleAbcImportFactory::GetPriority() const { return ImportPriority; }

#undef LOCTEXT_NAMESPACE
