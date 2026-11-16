
#include "DNACharacterImporterSettings.h"

UDNACharacterImporterSettings::UDNACharacterImporterSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("DNA Character Importer");
}

FName UDNACharacterImporterSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
