#include "Doodle/ContentBrowserMenuExtension.h"

/// 资产模块
#include "AssetToolsModule.h"
#include "AssetTools/Public/IAssetTools.h"
#include "AssetRegistryModule.h"

// 调整贴图大小
#include "ResizeTexture.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "DoodleGameEditor"
FContentBrowserMenuExtension::FContentBrowserMenuExtension(const TArray<FAssetData>& InPaths)
    : Paths(InPaths) {
}
void FContentBrowserMenuExtension::AddMenuEntry(FMenuBuilder& MenuBuilder) {
  // Create Section
  MenuBuilder.BeginSection("CustomMenu", LOCTEXT("PathViewOptionsMenuHeading", "Doodle"));
  {
    // Create a Submenu inside of the Section
    MenuBuilder.AddSubMenu(
        FText::FromString("Resize Texture"), FText::FromString("Resize Texture"),
        FNewMenuDelegate::CreateRaw(this, &FContentBrowserMenuExtension::FillSubmenu)
    );
  }
  MenuBuilder.EndSection();
}
void FContentBrowserMenuExtension::FillSubmenu(FMenuBuilder& MenuBuilder) {
  // Create the Submenu Entries
  MenuBuilder.AddMenuEntry(
      FText::FromString("Collate dependencies"),
      FText::FromString("Collate dependencies"),
      FSlateIcon(),
      FUIAction(FExecuteAction::CreateRaw(this, &FContentBrowserMenuExtension::OnRegenerateLODSClicked))
  );
}
void FContentBrowserMenuExtension::OnRegenerateLODSClicked() {
  FAssetToolsModule& AssetToolsModule       = FModuleManager::LoadModuleChecked<FAssetToolsModule>(FName("AssetTools"));
  IAssetTools& AssetTool                    = AssetToolsModule.Get();
  FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

  FResizeTexture L_Resize{};
  FScopedSlowTask L_Task_Scoped{(float_t)Paths.Num(), LOCTEXT("FResizeTexture", "开始转换...")};
  L_Task_Scoped.MakeDialog();
  for (const FAssetData& L_Ass : Paths) {
    if (UTexture2D* L_Tex = Cast<UTexture2D>(L_Ass.GetAsset())) {
      L_Resize.Resize(L_Tex);
    }
    L_Task_Scoped.EnterProgressFrame(1.0f);
    // TArray<FName> L_Sof_list{};
    // ;
    // AssetRegistryModule.GetDependencies(L_Ass.GetPackage()->GetFName(), L_Sof_list);
    // for (auto&& L_obj : L_Sof_list) {
    //   UE_LOG(LogTemp, Log, TEXT("确认引用 %s"), *(L_obj.ToString()));
    // }
  }
}

#undef LOCTEXT_NAMESPACE