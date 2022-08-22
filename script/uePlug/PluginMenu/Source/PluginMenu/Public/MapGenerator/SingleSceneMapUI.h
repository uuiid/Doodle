#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Widgets/SUserWidget.h"

class SSingleSceneMapUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSingleSceneMapUI)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments &InArgs);

	FString DefaultOpenFileDir = FPaths::GameContentDir();
	FString DefaultOpenFbxDir = "C:/";

	TArray<TSharedPtr<FString>> ItemsMap, ItemsScene, ItemsShot, ItemsEmpty;

	TSharedPtr<class SEditableTextBox> TextPorject, TextFbxDir, TextSceneMap;
	TSharedPtr<class SCheckBox> CheckBoxShot, CheckBoxMap;

	bool IsSaveInMap = true;
	bool MapExist(bool SaveInPath, FString Shot, FString MapName);
	FString SetMapName(FString Shot);
	void ItemsAddContent(FString ProjectPath);
	void ItemsUpdateContent();

	FReply OpenProjcetDir();
	FReply OpenFbxDir();
	FReply OpenSceneMap();

	// Save and Load Setting.json
	void WriteSetting();
	void LoadSetting();
	FReply SaveSetting();

	void SaveInMap(ECheckBoxState State);
	void SaveInShot(ECheckBoxState State);

	FReply CreateMap();
};