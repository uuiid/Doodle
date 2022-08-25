#include "DoodleImportUiltEditor.h"

#include "MovieSceneSection.h"

UDoodleImportUiltEditor *UDoodleImportUiltEditor::Get() {
  TArray<UClass *> ImportUiltClasses;
  GetDerivedClasses(UDoodleImportUiltEditor::StaticClass(), ImportUiltClasses);
  int32 NumClasses = ImportUiltClasses.Num();

  for (auto &&i : ImportUiltClasses) {
    // i->GetName();
    UE_LOG(LogTemp, Log, TEXT("get class name %s"), *i->GetName());
    if (i->GetName() == "DoodleImportUiltEditorimpl_C")
      return Cast<UDoodleImportUiltEditor>(i->GetDefaultObject());
  }

  return nullptr;
}
