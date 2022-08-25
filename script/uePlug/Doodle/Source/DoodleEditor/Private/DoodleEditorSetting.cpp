#include "DoodleEditorSetting.h"

UDoodleEditorSetting::UDoodleEditorSetting() {
  DoodleExePath = TEXT("doodle.exe");
}
#if WITH_EDITOR
FText UDoodleEditorSetting::GetSectionText() const {
  return FText::FromString(TEXT("doodle setting"));
}
FName UDoodleEditorSetting::GetCategoryName() const {
  return TEXT("Plugins");
}
#endif  // WITH_EDITOR
