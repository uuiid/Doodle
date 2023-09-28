#include "DoodleEditor.h"

#include "Doodle/ContentBrowserMenuExtension.h"
#include "Doodle/DoodleImportFbxUI.h"
#include "DoodleCommands.h"
#include "DoodleStyle.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "doodleCopyMaterial.h"

#include "ContentBrowserModule.h"  ///内容游览器
#include "Doodle/CreateCharacter/CreateCharacterMianUI.h"
#include "Doodle/CreateCharacter/CoreData/CreateCharacterActor.h"
#include "Doodle/CreateCharacter/Editor/CreateCharacterActor_Customization.h"
#include "AssetToolsModule.h"  // 注册资产动作
#include "Doodle/CreateCharacter/Editor/CreateCharacter_AssetTypeActions.h"
//--------------变体相关
#include "DoodleVariantAssetTypeActions.h"
#include "ISequencerModule.h"
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequence.h"
#include "Animation/SkeletalMeshActor.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "DoodleVariantAssetUserData.h"
#include "DoodleVariantCompoundWidget.h"

static const FName doodleTabName("doodleEditor");
#define LOCTEXT_NAMESPACE "FdoodleEditorModule"

void FdoodleEditorModule::StartupModule() {
  FdoodleStyle::Initialize();
  FdoodleStyle::ReloadTextures();

  FDoodleCommands::Register();

  PluginCommands = MakeShareable(new FUICommandList);
  /// @brief 注册命令
  PluginCommands->MapAction(
      FDoodleCommands::Get().OpenPluginWindow,
      FExecuteAction::CreateRaw(this, &FdoodleEditorModule::PluginButtonClicked), FCanExecuteAction()
  );
  PluginCommands->MapAction(
      FDoodleCommands::Get().DoodleImportFbxWindow,
      FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(SDoodleImportFbxUI::Name); }),
      FCanExecuteAction()
  );

  //zhanghang 23/09/25 变体相关代码
  PluginCommands->MapAction(
      FDoodleCommands::Get().DoodleVariantWindow,
      FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(DoodleVariantCompoundWidget::Name); }),
      FCanExecuteAction()
  );

  /// @brief 注册回调(在这里出现在工具菜单中)
  UToolMenus::RegisterStartupCallback(
      FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FdoodleEditorModule::RegisterMenus)
  );

  /// @brief 注册tab
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(doodleTabName, FOnSpawnTab::CreateRaw(this, &FdoodleEditorModule::OnSpawnPluginTab))
      .SetDisplayName(LOCTEXT("FdoodleTabTitle1", "Doodle"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(SDoodleImportFbxUI::Name, FOnSpawnTab::CreateStatic(&SDoodleImportFbxUI::OnSpawnAction))
      .SetDisplayName(LOCTEXT("FdoodleTabTitle2", "Doodle Import Fbx"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);
  //----------------zhanghang 变体相关 tab
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(DoodleVariantCompoundWidget::Name, FOnSpawnTab::CreateStatic(&DoodleVariantCompoundWidget::OnSpawnAction))
      .SetDisplayName(LOCTEXT("FdoodleTabTitle3", "Doodle Variant"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);

  FContentBrowserModule &ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
  TArray<FContentBrowserMenuExtender_SelectedAssets> &MenuExtenderDelegates =
      ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
  MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
      [this](const TArray<FAssetData> &Path) -> TSharedRef<FExtender> {
        // 创建一个扩展并保存数据
        Extension                          = MakeShareable(new FContentBrowserMenuExtension(Path));
        // 创建一个扩展 创建包含委托的扩展程序，该委托将被调用以获取有关新上下文菜单项的信息
        TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
        // 创建一个共享指针委托，该委托保留对对象“NewFolder”的弱引用。这是一个钩子名称，由扩展器用来标识将扩展路径上下文菜单的外部对象
        MenuExtender->AddMenuExtension(
            "GetAssetActions", EExtensionHook::After, TSharedPtr<FUICommandList>(),
            FMenuExtensionDelegate::CreateSP(Extension.ToSharedRef(), &FContentBrowserMenuExtension::AddMenuEntry)
        );
        return MenuExtender.ToSharedRef();
      }
  ));

  // 注册资产类别
  IAssetTools &L_AssetTools         = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
  EAssetTypeCategories::Type L_Type = L_AssetTools.RegisterAdvancedAssetCategory(
      FName{TEXT("Doodle Character")}, LOCTEXT("Doodle Character", "Doodle Character")
  );

  // 注册资产动作
  L_AssetTools.RegisterAssetTypeActions(
      CreateAssetActions.Add_GetRef(MakeShared<FAssetTypeActions_CreateCharacter>(L_Type)).ToSharedRef()
  );

  // 注册详细面板
  FPropertyEditorModule &L_Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
  L_Module.RegisterCustomClassLayout(ADoodleCreateCharacterActor::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&CreateCharacterActor_Customization::MakeInstance));
  L_Module.NotifyCustomizationModuleChanged();

  // AssetDataSource.Reset(NewObject<UContentBrowserAssetDataSource>(
  //    GetTransientPackage(), "doodle_AssetData"));
  // AssetDataSource->Initialize(R"(/doodle/test)");
  // auto AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
  //                          AssetRegistryConstants::ModuleName)
  //                          .Get();
  // AssetRegistry->AddPath(R"(/../../tmp2/Content/)");
  //---------------------注册 zhanghang 23/09/25 变体相关--------------------------------------------
  IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
  EAssetTypeCategories::Type AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName{ TEXT("Create Variant") }, LOCTEXT("Doodle", "Doodle Variant"));
  RegisterActionType = MakeShareable(new DoodleVariantAssetTypeActions(AssetCategory));
  AssetTools.RegisterAssetTypeActions(RegisterActionType.ToSharedRef());
  //-------------------------
  ISequencerModule& module = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
  TSharedPtr<FExtensibilityManager> Manager = module.GetObjectBindingContextMenuExtensibilityManager();

  module.RegisterOnSequencerCreated(FOnSequencerCreated::FDelegate::CreateLambda([this](TSharedRef<ISequencer> OwningSequencer) {

      TheSequencer = OwningSequencer.ToWeakPtr();
      }));
  //-----------------------
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([&] 
        {
        TSharedPtr<FExtender> extender = MakeShareable(new FExtender());
        extender->AddMenuExtension("Edit", EExtensionHook::Before, TSharedPtr<FUICommandList>(), FMenuExtensionDelegate::CreateLambda([&](FMenuBuilder& builder)
        {
                TSharedPtr<ISequencer> sequencer = TheSequencer.Pin();
                TArray<FGuid> Objects;
                sequencer.Get()->GetSelectedObjects(Objects);
                //--------------------------
                if (Objects.Num() > 0)
                {
                    AActor* actor = nullptr;
                    for (TWeakObjectPtr<UObject> ptr : sequencer.Get()->FindObjectsInCurrentSequence(Objects[0]))
                    {
                        actor =  Cast<AActor>(ptr.Get());
                        if (actor) { break; }
                    }
                    if (actor) 
                    {
                        TArray<UObject*> Assets;
                        actor->GetReferencedContentObjects(Assets);
                        if (Assets.Num() > 0 && Assets[0]->GetClass()->IsChildOf<USkeletalMesh>())
                        {
                            USkeletalMesh* mesh = Cast<USkeletalMesh>(Assets[0]);
                            UDoodleVariantAssetUserData* user_data = mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
                            if (user_data&& user_data->variantObj)
                            {
                                builder.BeginSection("Doodle Varaint", LOCTEXT("Varaint", "Varaint"));
                                {
                                    builder.AddSubMenu
                                    (
                                        FText::FromString(TEXT("切换变体")),
                                        FText::FromString(TEXT("切换变体 tooltip")),
                                        FNewMenuDelegate::CreateLambda([this, user_data, actor](FMenuBuilder& builder)
                                            {
                                                UDoodleVariantObject* myObject = user_data->variantObj;
                                                if (myObject)
                                                {
                                                    for (auto& e : myObject->allVaraint)
                                                    {
                                                        builder.AddMenuEntry(
                                                            FText::FromString(e.Key),
                                                            LOCTEXT("DoodleVaraintTooltip", "Change Skeletal Mesh Varaint"),
                                                            FSlateIcon(),
                                                            // NOTE 设置点击触发的函数
                                                            FUIAction(FExecuteAction::CreateLambda([myObject, e, actor]()
                                                                {
                                                                    myObject->allVaraint[e.Key];
                                                                    //----------------------
                                                                    ASkeletalMeshActor* mesh = Cast<ASkeletalMeshActor>(actor);
                                                                    TArray<FSkeletalMaterial> list = myObject->allVaraint[e.Key].varaints;
                                                                    for (int i = 0;i < list.Num();i++)
                                                                    {
                                                                        mesh->GetSkeletalMeshComponent()->SetMaterial(i, list[i].MaterialInterface);
                                                                    }
                                                                    mesh->GetSkeletalMeshComponent()->PostApplyToComponent();
                                                                })));
                                                    }
                                                }
                                            }),
                                        FUIAction(),
                                        NAME_None,
                                        EUserInterfaceActionType::Button,
                                        false,
                                        FSlateIcon()
                                    );
                                }
                                builder.EndSection();
                            }
                        }
                    }
                }
        }));
        ISequencerModule& module = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
        TSharedPtr<FExtensibilityManager> Manager = module.GetObjectBindingContextMenuExtensibilityManager();
        Manager.Get()->AddExtender(extender);
        }));
}

void FdoodleEditorModule::ShutdownModule() {
  UToolMenus::UnRegisterStartupCallback(this);

  UToolMenus::UnregisterOwner(this);

  FdoodleStyle::Shutdown();

  FDoodleCommands::Unregister();

  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(doodleTabName);
  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SDoodleImportFbxUI::Name);
  // zhanghang 变体相关 23/09/25
  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DoodleVariantCompoundWidget::Name);

  // 取消注册资产动作
  if (FModuleManager::Get().IsModuleLoaded("AssetTools")) {
    IAssetTools &L_AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    for (auto &&i : CreateAssetActions)
      L_AssetTools.UnregisterAssetTypeActions(
          i.ToSharedRef()
      );
  }
  CreateAssetActions.Empty();
  // 详细面板
  if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")) {
    FPropertyEditorModule &L_Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    L_Module.UnregisterCustomClassLayout(ADoodleCreateCharacterActor::StaticClass()->GetFName());
    L_Module.NotifyCustomizationModuleChanged();
  }
  // AssetDataSource.Reset();
  //-------------取消注册 zhanghang 变体相关 23/09/25
  if (FModuleManager::Get().IsModuleLoaded("AssetTools")) {
      IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
      if(RegisterActionType)
      AssetTools.UnregisterAssetTypeActions(RegisterActionType.ToSharedRef());
  }
}
TSharedRef<SDockTab> FdoodleEditorModule::OnSpawnPluginTab(
    const FSpawnTabArgs &SpawnTabArgs
) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[
      // Put your tab content here!
      SNew(SBox)
          .HAlign(HAlign_Left)
          .VAlign(VAlign_Top)[SNew(DoodleCopyMat)  // 这里创建我们自己的界面
  ]];
}

void FdoodleEditorModule::PluginButtonClicked() {
#if ENGINE_MINOR_VERSION == 27
  FGlobalTabmanager::Get()->TryInvokeTab(doodleTabName);
#else if ENGINE_MINOR_VERSION < 27
  FGlobalTabmanager::Get()->TryInvokeTab(doodleTabName);
#endif
}

void FdoodleEditorModule::RegisterMenus() {
  // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
  FToolMenuOwnerScoped OwnerScoped(this);

  {
    UToolMenu *Menu =
        UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
      FToolMenuSection &Section = Menu->FindOrAddSection("WindowLayout");
      Section.AddMenuEntryWithCommandList(FDoodleCommands::Get().OpenPluginWindow, PluginCommands);
      Section.AddMenuEntryWithCommandList(FDoodleCommands::Get().DoodleImportFbxWindow, PluginCommands);
      Section.AddMenuEntryWithCommandList(FDoodleCommands::Get().DoodleVariantWindow, PluginCommands);
    }
  }

  {
    UToolMenu *ToolbarMenu =
        UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
    {
      FToolMenuSection &Section = ToolbarMenu->FindOrAddSection("Settings");
      {
        FToolMenuEntry &Entry =
            Section.AddEntry(FToolMenuEntry::InitToolBarButton(
                FDoodleCommands::Get().OpenPluginWindow
            ));
        Entry.SetCommandList(PluginCommands);
      }
    }
  }
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FdoodleEditorModule, doodleEditor)
