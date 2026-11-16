// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "DNACharacterImporterTypes.h"

/**
 * Entry point of the DNA Character Importer plugin.
 *
 * A DNA character FBX carries hundreds of blend shapes. The engine importer builds a complete mesh
 * description for every one of them, which is what makes importing such a file so memory hungry.
 * This importer imports the mesh without morph targets and rebuilds them one shape at a time.
 */
class DOODLEDNAIMPORTER_API FDNACharacterImporter
{
public:
	/**
	 * Runs one import. Must be called on the game thread.
	 *
	 * @param Request  What to import and how.
	 * @param OutResult Everything that happened, including the memory numbers.
	 * @return true when the mesh (and, unless the mode says otherwise, the morph targets) is ready.
	 */
	static bool Run(const FDNACharacterImportRequest& Request, FDNACharacterImportResult& OutResult);

	/** Fills in the fields the caller left empty (asset name, destination path, absolute file name). */
	static void NormalizeRequest(FDNACharacterImportRequest& Request);

	/** Human readable name of an import mode, for logs and command lines. */
	static FString ModeToString(EDNACharacterImportMode Mode);

	/** Parses "stock" / "lowmem" / "morphs" (case insensitive). Returns false when unknown. */
	static bool ParseMode(const FString& Text, EDNACharacterImportMode& OutMode);
};
