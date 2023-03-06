#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
#include "CreateCharacterTree.h"

#define LOCTEXT_NAMESPACE "FCreateCharacterMianUI"

const FName FCreateCharacterMianUI::TreeID{TEXT("Doodle_TreeID")};
const FName FCreateCharacterMianUI::ViewportID{TEXT("Doodle_ViewportID")};
const FName FCreateCharacterMianUI::AppIdentifier{TEXT("Doodle_AppIdentifier")};

void FCreateCharacterMianUI::RegisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) {
  WorkspaceMenuCategory = In_TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenuCategory", "Create Character Editor"));

  FAssetEditorToolkit::RegisterTabSpawners(In_TabManager);

  In_TabManager->RegisterTabSpawner(
                   FCreateCharacterMianUI::ViewportID,
                   FOnSpawnTab::CreateSP(this, &FCreateCharacterMianUI::SpawnTab_Viewport)
  )
      .SetDisplayName(LOCTEXT("WorkspaceMenuCategoryViewportID", "Viewport"))
      // clang-format off
      .SetGroup(WorkspaceMenuCategory.ToSharedRef())
      .SetIcon(FSlateIcon{FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"})
      // clang-format on
      ;

  In_TabManager->RegisterTabSpawner(FCreateCharacterMianUI::TreeID, FOnSpawnTab::CreateSP(this, &FCreateCharacterMianUI::SpawnTab_Tree))
      .SetDisplayName(LOCTEXT("TreeTabLabel", "Tree"))
      .SetGroup(WorkspaceMenuCategory.ToSharedRef())
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
}

void FCreateCharacterMianUI::UnregisterTabSpawners(const TSharedRef<FTabManager>& In_TabManager) {
  FAssetEditorToolkit::UnregisterTabSpawners(In_TabManager);
  In_TabManager->UnregisterTabSpawner(FCreateCharacterMianUI::ViewportID);
  In_TabManager->UnregisterTabSpawner(FCreateCharacterMianUI::TreeID);
}

FName FCreateCharacterMianUI::GetToolkitFName() const {
  return FName{TEXT("CreateCharacterMianUI")};
}

FText FCreateCharacterMianUI::GetBaseToolkitName() const {
  return LOCTEXT("GetBaseToolkitName", "Create Character Mian Editor");
}

FText FCreateCharacterMianUI::GetToolkitName() const {
  auto L_Dirty = CreateCharacterConfig->GetPackage()->IsDirty();
  FFormatNamedArguments Args{
      {TEXT("CreateCharacterConfigName"), FText::FromString(CreateCharacterConfig->GetName())},
      {TEXT("DirtyState"), L_Dirty ? FText::FromString(TEXT("*")) : FText::GetEmpty()}};

  return FText::Format(LOCTEXT("GetToolkitName", "{CreateCharacterConfigName}{DirtyState}"), Args);
}

FText FCreateCharacterMianUI::GetToolkitToolTipText() const {
  return FAssetEditorToolkit::GetToolTipTextForObject(CreateCharacterConfig);
}

FLinearColor FCreateCharacterMianUI::GetWorldCentricTabColorScale() const {
  return FLinearColor::White;
}

FString FCreateCharacterMianUI::GetWorldCentricTabPrefix() const {
  return TEXT("Create Character Mian Editor");
}

FString FCreateCharacterMianUI::GetDocumentationLink() const {
  return TEXT("Create Character Mian UI");
}

void FCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& Collector) {
}

void FCreateCharacterMianUI::InitCreateCharacterMianUI(
    EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& In_InitToolkitHost,
    UDoodleCreateCharacterConfig* In_Config
) {
  CreateCharacterConfig                = In_Config;
  UAssetEditorSubsystem* L_EditorAsset = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
  L_EditorAsset->CloseOtherEditors(In_Config, this);

  const TSharedRef<FTabManager::FLayout> L_Layout =
      FTabManager::NewLayout(
          "FCreateCharacterMianUI_InitCreateCharacterMianUI_Layout"
      )
          ->AddArea(
              // clang-format off
              FTabManager::NewPrimaryArea()
              ->SetOrientation(Orient_Vertical)
              ->Split
              (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Horizontal)
                ->SetSizeCoefficient(0.9f)
                ->Split
                (
                  FTabManager::NewStack()
                  ->SetSizeCoefficient(1.f)
                  ->SetHideTabWell(true)
                  ->AddTab(FCreateCharacterMianUI::ViewportID, ETabState::OpenedTab)
                )
                ->Split
                (
                  FTabManager::NewStack()
                  ->SetSizeCoefficient(1.f)
                  ->SetHideTabWell(true)
                  ->AddTab(FCreateCharacterMianUI::TreeID, ETabState::OpenedTab)
                )
              )
              // clang-format on
          );

  InitAssetEditor(Mode, In_InitToolkitHost, AppIdentifier, L_Layout, true, true, In_Config);

  TSharedPtr<FExtender> L_ToolbarExtender = MakeShared<FExtender>();
  L_ToolbarExtender->AddToolBarExtension(
      TEXT("Asset"),
      EExtensionHook::After,
      GetToolkitCommands(), /*ViewportPtr->GetCommandList()*/
      FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& In_ToolBarBuilder) {})
  );

  AddToolbarExtender(L_ToolbarExtender);

  RegenerateMenusAndToolbars();
}

TSharedRef<SDockTab> FCreateCharacterMianUI::SpawnTab_Viewport(const FSpawnTabArgs& Args) {
  // clang-format off
  return SNew(SDockTab)
    .Label(LOCTEXT("SpawnTab_Viewport","Viewport"))
    [
      SNew(SVerticalBox)
      
      + SVerticalBox::Slot()
      [
        SAssignNew(CharacterEditorViewport, SCharacterEditorViewport)
        .DoodleCreateCharacterConfigAttr(CreateCharacterConfig)
      ]
      //+ SVerticalBox::Slot()
      //.Padding(0,8,0,0)
      //.AutoHeight()
      //.HAlign(HAlign_Fill)
      //[
      //]
    ];
  // clang-format on
}

TSharedRef<SDockTab> FCreateCharacterMianUI::SpawnTab_Tree(const FSpawnTabArgs& Args) {
  // clang-format off
  return SNew(SDockTab)
    .Label(LOCTEXT("SpawnTab_Tree","Tree"))
    [
      SNew(SVerticalBox)
      
      + SVerticalBox::Slot()
      [
        SAssignNew(CreateCharacterTree, SCreateCharacterTree)
        .CreateCharacterConfig(CreateCharacterConfig)
        //+ SVerticalBox::Slot()
        //.Padding(0,8,0,0)
        //.AutoHeight()
        //.HAlign(HAlign_Fill)
        //[
        //]
      ]
  ];
  // clang-format on
}

#undef LOCTEXT_NAMESPACE