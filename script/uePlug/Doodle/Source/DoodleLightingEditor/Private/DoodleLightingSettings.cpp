#include "DoodleLightingSettings.h"

FName UDoodleLightingSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FText UDoodleLightingSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Doodle Lighting"));
}
