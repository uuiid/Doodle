// Copyright Epic Games, Inc. All Rights Reserved.

#include "DNACharacterPhysicsAsset.h"

#include "DNACharacterImporterModule.h"
#include "DNACharacterImporterSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace DNACharacterImporterInternal
{
	bool CreatePhysicsAssetForMesh(USkeletalMesh& Mesh, const UDNACharacterImporterSettings& Settings, FString& OutPhysicsAssetPath, FString& OutError)
	{
		OutPhysicsAssetPath.Reset();
		OutError.Reset();

		if (!Settings.bCreatePhysicsAsset)
		{
			return true;
		}

		// A mesh that already points at a physics asset keeps it: re-running the import must not
		// throw away a hand edited asset.
		if (const UPhysicsAsset* ExistingPhysicsAsset = Mesh.GetPhysicsAsset())
		{
			OutPhysicsAssetPath = ExistingPhysicsAsset->GetPathName();
			UE_LOG(LogDoodleDnaImporter, Display,
				TEXT("The skeletal mesh already has the physics asset '%s'; it was left untouched."), *OutPhysicsAssetPath);
			return true;
		}

		const FString MeshPackageName = Mesh.GetOutermost()->GetName();
		const FString MeshPackagePath = FPackageName::GetLongPackagePath(MeshPackageName);
		const FString PhysicsAssetName = FString::Printf(TEXT("%s_PhysicsAsset"), *Mesh.GetName());
		const FString PhysicsAssetPackageName = FString::Printf(TEXT("%s/%s"), *MeshPackagePath, *PhysicsAssetName);

		UPackage* PhysicsAssetPackage = CreatePackage(*PhysicsAssetPackageName);
		if (PhysicsAssetPackage == nullptr)
		{
			OutError = FString::Printf(TEXT("The package '%s' could not be created."), *PhysicsAssetPackageName);
			return false;
		}

		UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(FindObject<UObject>(PhysicsAssetPackage, *PhysicsAssetName));
		if (PhysicsAsset == nullptr)
		{
			if (FindObject<UObject>(PhysicsAssetPackage, *PhysicsAssetName) != nullptr)
			{
				OutError = FString::Printf(TEXT("'%s' already exists but is not a physics asset."), *PhysicsAssetPackageName);
				return false;
			}

			PhysicsAsset = NewObject<UPhysicsAsset>(PhysicsAssetPackage, *PhysicsAssetName, RF_Public | RF_Standalone | RF_Transactional);
			if (PhysicsAsset == nullptr)
			{
				OutError = FString::Printf(TEXT("The physics asset '%s' could not be created."), *PhysicsAssetPackageName);
				return false;
			}
		}

		// Same body generation the engine uses for a regular FBX import, with the bone size floor from
		// the plugin settings so the hundreds of tiny facial bones do not each get a body.
		FPhysAssetCreateParams CreateParams;
		CreateParams.MinBoneSize = Settings.PhysicsAssetMinBoneSize;
		CreateParams.LodIndex = Settings.PhysicsAssetLodIndex;

		FText ErrorMessage;
		const bool bCreated = FPhysicsAssetUtils::CreateFromSkeletalMesh(
			PhysicsAsset,
			&Mesh,
			CreateParams,
			ErrorMessage,
			/*bSetToMesh=*/false,
			/*bShowProgress=*/false);

		if (!bCreated)
		{
			OutError = ErrorMessage.IsEmpty()
				? FString::Printf(TEXT("The bodies of '%s' could not be generated."), *PhysicsAssetName)
				: ErrorMessage.ToString();
			PhysicsAsset->MarkAsGarbage();
			return false;
		}

		Mesh.SetPhysicsAsset(PhysicsAsset);
		PhysicsAsset->MarkPackageDirty();
		Mesh.MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(PhysicsAsset);

		OutPhysicsAssetPath = PhysicsAsset->GetPathName();
		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("Created the physics asset '%s' (%d body/bodies, bone size floor %.1f cm)."),
			*OutPhysicsAssetPath, PhysicsAsset->SkeletalBodySetups.Num(), CreateParams.MinBoneSize);
		return true;
	}
}
