// Copyright Epic Games, Inc. All Rights Reserved.

#include "Doodle/Abc/DoodleAbcImportSettings.h"

#include "Serialization/Archive.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/ReleaseObjectVersion.h"

UDoodleAbcImportSettings::UDoodleAbcImportSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  bReimport = false;
}
void UDoodleAbcImportSettings::Serialize(FArchive& Archive) {
  Super::Serialize(Archive);

  Archive.UsingCustomVersion(FReleaseObjectVersion::GUID);

  if (Archive.IsLoading() &&
      Archive.CustomVer(FReleaseObjectVersion::GUID) < FReleaseObjectVersion::AbcVelocitiesSupport) {
  }
}
