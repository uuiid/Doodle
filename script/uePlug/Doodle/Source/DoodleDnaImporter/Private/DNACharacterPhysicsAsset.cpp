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
	TValueOrError<FString, FPhysicsAssetError> CreatePhysicsAssetForMesh(
		USkeletalMesh& InMesh, const UDNACharacterImporterSettings& InSettings)
	{
		if (!InSettings.bCreatePhysicsAsset)
		{
			return MakeValue(FString());
		}

		if (const UPhysicsAsset* ExistingPhysicsAsset = InMesh.GetPhysicsAsset())
		{
			const FString ExistingPath = ExistingPhysicsAsset->GetPathName();
			UE_LOG(LogDoodleDnaImporter, Display,
				TEXT("The skeletal mesh already has the physics asset '%s'; it was left untouched."), *ExistingPath);
			return MakeValue(ExistingPath);
		}

		const FString MeshPackageName = InMesh.GetOutermost()->GetName();
		const FString MeshPackagePath = FPackageName::GetLongPackagePath(MeshPackageName);
		const FString PhysicsAssetName = FString::Printf(TEXT("%s_PhysicsAsset"), *InMesh.GetName());
		const FString PhysicsAssetPackageName = FString::Printf(TEXT("%s/%s"), *MeshPackagePath, *PhysicsAssetName);

		UPackage* PhysicsAssetPackage = CreatePackage(*PhysicsAssetPackageName);
		if (PhysicsAssetPackage == nullptr)
		{
			return MakeError(FPhysicsAssetError{
				EPhysicsAssetError::PackageCreationFailed,
				FString::Printf(TEXT("The package '%s' could not be created."), *PhysicsAssetPackageName)
			});
		}

		UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(FindObject<UObject>(PhysicsAssetPackage, *PhysicsAssetName));
		if (PhysicsAsset == nullptr)
		{
			if (FindObject<UObject>(PhysicsAssetPackage, *PhysicsAssetName) != nullptr)
			{
				return MakeError(FPhysicsAssetError{
					EPhysicsAssetError::NameOccupied,
					FString::Printf(TEXT("'%s' already exists but is not a physics asset."), *PhysicsAssetPackageName)
				});
			}

			PhysicsAsset = NewObject<UPhysicsAsset>(PhysicsAssetPackage, *PhysicsAssetName, RF_Public | RF_Standalone | RF_Transactional);
			if (PhysicsAsset == nullptr)
			{
				return MakeError(FPhysicsAssetError{
					EPhysicsAssetError::AssetCreationFailed,
					FString::Printf(TEXT("The physics asset '%s' could not be created."), *PhysicsAssetPackageName)
				});
			}
		}

		FPhysAssetCreateParams CreateParams;
		CreateParams.MinBoneSize = InSettings.PhysicsAssetMinBoneSize;
		CreateParams.LodIndex = InSettings.PhysicsAssetLodIndex;

		FText ErrorMessage;
		const bool bCreated = FPhysicsAssetUtils::CreateFromSkeletalMesh(
			PhysicsAsset,
			&InMesh,
			CreateParams,
			ErrorMessage,
			false,
			false);

		if (!bCreated)
		{
			PhysicsAsset->MarkAsGarbage();
			return MakeError(FPhysicsAssetError{
				EPhysicsAssetError::BodyGenerationFailed,
				ErrorMessage.IsEmpty()
					? FString::Printf(TEXT("The bodies of '%s' could not be generated."), *PhysicsAssetName)
					: ErrorMessage.ToString()
			});
		}

		InMesh.SetPhysicsAsset(PhysicsAsset);
		PhysicsAsset->MarkPackageDirty();
		InMesh.MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(PhysicsAsset);

		const FString PhysicsAssetPath = PhysicsAsset->GetPathName();
		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("Created the physics asset '%s' (%d body/bodies, bone size floor %.1f cm)."),
			*PhysicsAssetPath, PhysicsAsset->SkeletalBodySetups.Num(), CreateParams.MinBoneSize);
		return MakeValue(PhysicsAssetPath);
	}
}
