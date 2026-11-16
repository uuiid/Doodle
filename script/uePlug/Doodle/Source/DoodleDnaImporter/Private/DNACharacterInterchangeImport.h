// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "DNACharacterImporterTypes.h"

class UDNACharacterImporterSettings;

namespace DNACharacterImporterInternal
{
	struct FImportedMeshInfo
	{
		FString MeshObjectPath;
		FString SkeletonObjectPath;
	};

	/**
	 * Imports the skeletal mesh of an FBX with the engine's Interchange pipeline.
	 *
	 * Only the base mesh is imported: materials, textures, animations and LODs are switched off, and
	 * bImportMorphTargets decides whether Interchange builds the blend shapes itself (the stock
	 * behaviour, very memory hungry) or leaves them to this plugin.
	 */
	bool ImportSkeletalMeshFromFbx(
		const FDNACharacterImportRequest& Request,
		const UDNACharacterImporterSettings& Settings,
		bool bImportMorphTargets,
		FImportedMeshInfo& OutInfo,
		FString& OutError);
}
