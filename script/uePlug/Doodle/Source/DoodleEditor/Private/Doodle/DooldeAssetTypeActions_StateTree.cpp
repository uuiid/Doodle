#include "DooldeAssetTypeActions_StateTree.h"

UClass* FDoodleAssetTypeActions_StateTree::GetSupportedClass() const {
  return nullptr;
}

void FDoodleAssetTypeActions_StateTree::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
}

uint32 FDoodleAssetTypeActions_StateTree::GetCategories() {
  return uint32();
}
