#include "AbcWrap/DoodleAbcImportSetting.h"
#include "UObject/Class.h"
#include "UObject/Package.h"

UAbcDoodleImportSettings::UAbcDoodleImportSettings(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    ImportType = EDooAlembicImportType::GeometryCache;
    bReimport = false;
}

UAbcDoodleImportSettings *UAbcDoodleImportSettings::Get()
{
    static UAbcDoodleImportSettings *DefaultSettings = nullptr;
    if (!DefaultSettings)
    {
        // This is a singleton, use default object
        DefaultSettings = DuplicateObject(GetMutableDefault<UAbcDoodleImportSettings>(), GetTransientPackage());
        DefaultSettings->AddToRoot();
    }

    return DefaultSettings;
}