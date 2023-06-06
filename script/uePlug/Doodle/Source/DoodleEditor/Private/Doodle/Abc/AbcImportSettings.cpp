// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbcImportSettings.h"

#include "Serialization/Archive.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/ReleaseObjectVersion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbcImportSettings)

void UDoodleAbcImportSettings::Serialize(FArchive& Archive) {
  Super::Serialize(Archive);

  Archive.UsingCustomVersion(FReleaseObjectVersion::GUID);

  if (Archive.IsLoading() &&
      Archive.CustomVer(FReleaseObjectVersion::GUID) < FReleaseObjectVersion::AbcVelocitiesSupport) {
  }
}
