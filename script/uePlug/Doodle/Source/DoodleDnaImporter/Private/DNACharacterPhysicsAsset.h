// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDNACharacterImporterSettings;
class USkeletalMesh;

namespace DNACharacterImporterInternal
{
	/**
	 * Creates the physics asset of an imported DNA character next to its mesh.
	 *
	 * The asset is named "<MeshName>_PhysicsAsset" and holds one auto generated body per bone that is
	 * large enough (see UDNACharacterImporterSettings::PhysicsAssetMinBoneSize). It is created after
	 * the mesh has been rebuilt, which is what makes the bodies match the geometry that is saved, and
	 * it is linked to the mesh on success.
	 *
	 * Re-importing a mesh that already has a physics asset leaves that asset alone and only reports it.
	 *
	 * @param	Mesh				Skeletal mesh the bodies are generated from. Must be fully built.
	 * @param	Settings			Plugin settings (bCreatePhysicsAsset, PhysicsAssetMinBoneSize, ...).
	 * @param	OutPhysicsAssetPath	Object path of the physics asset (empty when creation is turned off).
	 * @param	OutError			Why the physics asset could not be created.
	 * @return	false only when the asset was requested but could not be created.
	 */
	bool CreatePhysicsAssetForMesh(USkeletalMesh& Mesh, const UDNACharacterImporterSettings& Settings, FString& OutPhysicsAssetPath, FString& OutError);
}
