#include "Doodle/ContentBrowserMenuExtension.h"

#include "Engine/SkeletalMeshLODSettings.h"

/// 资产模块
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

// 调整贴图大小
#include "ResizeTexture.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"
// 寻找类
#include "UObject/ConstructorHelpers.h"

// 生成骨骼lod
#include "Animation/DebugSkelMeshComponent.h"
#include "LODUtilities.h"
//------------------------------
#include "DoodleEffectLibraryEditWidget.h"
#include "NiagaraSystem.h"

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
        FText::FromString("Doodle"), FText::FromString("Doodle"),
        FNewMenuDelegate::CreateRaw(this, &FContentBrowserMenuExtension::FillSubmenu)
    );
  }
  MenuBuilder.EndSection();
}
void FContentBrowserMenuExtension::FillSubmenu(FMenuBuilder& MenuBuilder) {
  // Create 调整贴图大小的菜单
  MenuBuilder.AddMenuEntry(
      FText::FromString("Resize Texture"),
      FText::FromString("Resize Texture"),
      FSlateIcon(),
      FUIAction(FExecuteAction::CreateRaw(this, &FContentBrowserMenuExtension::ResizeTexTure))
  );

  MenuBuilder.AddMenuEntry(
      FText::FromString("Create LODS"),
      FText::FromString("Create LODS"),
      FSlateIcon(),
      FUIAction(FExecuteAction::CreateRaw(this, &FContentBrowserMenuExtension::OnRegenerateLODSClicked))
  );
//-------------------------
  MenuBuilder.AddMenuEntry(
      FText::FromString(TEXT("导入到特效库")),
      FText::FromString(TEXT("导入到特效库")),
      FSlateIcon(),
      FUIAction(FExecuteAction::CreateRaw(this, &FContentBrowserMenuExtension::OnImportToEffectLibrary))
  );
}
void FContentBrowserMenuExtension::OnRegenerateLODSClicked() {
  ;
  for (const FAssetData& L_Ass : Paths) {
    if (UStaticMesh* L_Mesh = Cast<UStaticMesh>(L_Ass.GetAsset())) {
      if (L_Mesh->LODGroup == FName{"None"})
        L_Mesh->SetLODGroup(FName{TEXT("HighDetail")});
    }
    if (USkeletalMesh* L_Skin = Cast<USkeletalMesh>(L_Ass.GetAsset())) {
      if (!L_Skin->GetLODSettings()) {
        BoildSkinLODS(L_Skin);
      }
    }
  }
}
void FContentBrowserMenuExtension::ResizeTexTure() {
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
  }
}

void FContentBrowserMenuExtension::BoildSkinLODS(USkeletalMesh* In_Skin_Mesh) {
  check(In_Skin_Mesh);
  static USkeletalMeshLODSettings* L_Skin_Mesh_Setting = LoadObject<USkeletalMeshLODSettings>(GetTransientPackage(), TEXT("/Doodle/Doodle_LOD_Setting.Doodle_LOD_Setting"));
  In_Skin_Mesh->SetLODSettings(L_Skin_Mesh_Setting);
  FScopedSuspendAlternateSkinWeightPreview ScopedSuspendAlternateSkinnWeightPreview(In_Skin_Mesh);
  {
    FScopedSkeletalMeshPostEditChange ScopedPostEditChange(In_Skin_Mesh);
    check(In_Skin_Mesh);

    FLODUtilities::RegenerateLOD(In_Skin_Mesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 5, false, true);
    In_Skin_Mesh->PostEditChange();
    In_Skin_Mesh->MarkPackageDirty();
  }
}

void FContentBrowserMenuExtension::OnImportToEffectLibrary()
{
    if (Paths.Num() > 0) 
    {
        const FAssetData& SelectAssetData = Paths.Top();
        if (SelectAssetData.GetAsset()->IsA<UParticleSystem>()|| SelectAssetData.GetAsset()->IsA<UNiagaraSystem>()
            || SelectAssetData.GetAsset()->IsA<UBlueprint>())
        {
            //----------------------
            FString LibraryPath;
            if (GConfig->GetString(TEXT("DoodleEffectLibrary"), TEXT("EffectLibraryPath"), LibraryPath, GEngineIni)) 
            {
                TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(UDoodleEffectLibraryEditWidget::Name);
                TSharedRef<UDoodleEffectLibraryEditWidget> Widget = StaticCastSharedRef<UDoodleEffectLibraryEditWidget>(Tab->GetContent());
                Widget->SetAssetData(SelectAssetData);
            }
            else
            {
                FText  DialogText = FText::FromString(TEXT("没有特效库路径，请在预览界面添加"));
                FMessageDialog::Open(EAppMsgType::Ok, DialogText);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE