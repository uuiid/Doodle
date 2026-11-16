// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDoodleDnaImporter, Log, All);

/**
 * Runtime module of the DNA Character Importer plugin.
 *
 * It hosts everything that does not need editor only APIs:
 *  - the plugin settings (UDNACharacterImporterSettings)
 *  - the memory trace helper (FDNAMemoryTrace)
 *  - a small wrapper around the vendored ufbx parser (FDNAFbxShapeSource) that can read the
 *    blend shapes of an FBX file without building a mesh description for every shape.
 */
class FDNACharacterImporterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
