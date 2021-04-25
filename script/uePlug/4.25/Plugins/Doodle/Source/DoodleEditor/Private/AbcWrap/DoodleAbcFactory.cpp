// Fill out your copyright notice in the Description page of Project Settings.

#include "AbcWrap/DoodleAbcFactory.h"
#include "AbcWrap/DoodleGeometryBonesActor.h"
#include "AbcWrap/DoodleAbcImport.h"
#include "AbcWrap/DoodleAbcImportSetting.h"

#include "GeometryCache.h"
#include "Editor.h"

#include "AssetImportTask.h"

#include "AbcWrap/DoodleAbcAssetImportData.h"

#include "Subsystems/ImportSubsystem.h"
#include "Engine/StaticMesh.h"
UDoodleAbcFactory::UDoodleAbcFactory()
	: UFactory(),
	  ImportSettings(NewObject<UAbcDoodleImportSettings>())
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = nullptr;

	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("abc;Doodle_Alembic"));
	ImportPriority = 99.f;
	AssetImportTask = NewObject<UAssetImportTask>();
}

void UDoodleAbcFactory::PostInitProperties()
{
	Super::PostInitProperties();

	ImportSettings = UAbcDoodleImportSettings::Get();
}

FText UDoodleAbcFactory::GetDisplayName() const
{
	return NSLOCTEXT("doodle", "doodleAbc", "Alembic");
}

bool UDoodleAbcFactory::DoesSupportClass(UClass *Class)
{
	return (Class == UGeometryCache::StaticClass());
}

UClass *UDoodleAbcFactory::ResolveSupportedClass()
{
	return UStaticMesh::StaticClass();
}

bool UDoodleAbcFactory::FactoryCanImport(const FString &FileName)
{
	//这里为什么有这么一句我也不知道为什么
	const FString suff = FPaths::GetExtension(FileName);

	return (FPaths::GetExtension(FileName) == TEXT("abc"));
}

UObject *UDoodleAbcFactory::FactoryCreateFile(UClass *InClass, UObject *InParent, FName InName,
											  EObjectFlags Flags, const FString &Filename, const TCHAR *Parms, FFeedbackContext *Warn,
											  bool &bOutOperationCanceled)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, TEXT("doodle_abc"));

	FDoodleAbcImport Import{};

	auto errorCode = Import.OpenAbcFileForImport(Filename);

	ImportSettings->bReimport = false;

	AdditionalImportedObjects.Empty();
	if (!errorCode)
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
	ImportSettings->SamplingSettings.FrameStart = 1001;
	ImportSettings->SamplingSettings.FrameEnd = Import.GetEndFrameIndex();
	bOutOperationCanceled = false;
	//这个是用来获得ui的设置的, 直接注释掉
	//UAbcImportSettings* scriptedStting = AssetImportTask ? Cast<UAbcImportSettings>(AssetImportTask->Options) : nullptr;
	//if (scriptedStting)
	//{
	//    ImportSettings = scriptedStting;
	//}
	//输出日志
	const FString PageName = "Importing " + InName.ToString() + ".abc";

	TArray<UObject *> ResultAssets;
	if (!bOutOperationCanceled)
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, TEXT("ABC"));

		int32 NumThreads = 1;
		if (FPlatformProcess::SupportsMultithreading())
		{
			NumThreads = FPlatformMisc::NumberOfCores();
		}
		//导入文件
		errorCode = Import.ImportTrackData(NumThreads, ImportSettings);

		if (!errorCode)
		{
			GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
			// FAbcImportLogger::OutputMessages(PageName);
			return nullptr;
		}
		else
		{ //导入几何缓存
			if (ImportSettings->ImportType == EDooAlembicImportType::GeometryCache)
			{
				UObject *GeometryCache = ImportGeometryCache(Import, InParent, Flags);
				if (GeometryCache)
				{
					ResultAssets.Add(GeometryCache);
				}
			}
		}

		AdditionalImportedObjects.Reserve(ResultAssets.Num());
		for (UObject *obj : ResultAssets)
		{
			if (obj)
			{
				GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, obj);
				obj->MarkPackageDirty();
				obj->PostEditChange();
				AdditionalImportedObjects.Add(obj);
			}
		}

		// FAbcImportLogger::OutputMessages(PageName);
	}
	//确认父级;
	UObject *OutParent = (ResultAssets.Num() > 0 && InParent != ResultAssets[0]->GetOutermost()) ? ResultAssets[0]->GetOutermost() : InParent;
	return (ResultAssets.Num() > 0) ? OutParent : nullptr;
}

UObject *UDoodleAbcFactory::ImportGeometryCache(FDoodleAbcImport &Importer, UObject *InParent, EObjectFlags Flags)
{
	// Flush commands before importing
	FlushRenderingCommands();

	const uint32 NumMeshes = Importer.GetNumMeshTracks();
	// Check if the alembic file contained any meshes
	if (NumMeshes > 0)
	{
		UDoodleGeometryCacheBones *GeometryCache = Importer.ImportAsDoodleGeometryCache(InParent, Flags);
		//if (Cast<UDoodleAlemblcCache>(GeometryCache) == nullptr)
		//{
		//	UE_LOG(LogTemp, Log, TEXT("tran doodle Cache ------------>   not"));
		//}

		if (!GeometryCache)
		{
			return nullptr;
		}

		// Setup asset import data
		if (!GeometryCache->p_GeometryCache->AssetImportData ||
			!GeometryCache->p_GeometryCache->AssetImportData->IsA<UDoodleAbcAssetImportData>())
		{
			GeometryCache->p_GeometryCache->AssetImportData = NewObject<UDoodleAbcAssetImportData>(GeometryCache->p_GeometryCache);
		}
		GeometryCache->p_GeometryCache->AssetImportData->Update(UFactory::CurrentFilename);
		UDoodleAbcAssetImportData *AssetImportData = Cast<UDoodleAbcAssetImportData>(GeometryCache->p_GeometryCache->AssetImportData);
		if (AssetImportData)
		{
			Importer.UpdateAssetImportData(AssetImportData);
		}
		return GeometryCache;
	}
	else
	{
		// Not able to import a static mesh
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
}
