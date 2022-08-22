#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SUserWidget.h"

#include "MapGenerator/DataType.h"

class SEditableTextBox;

class FUIOperationUtils
{
public:
	static void ChooseProjectFolderAndDisplay(TSharedPtr<SEditableTextBox> TextBox, FString &DefaultOpenDirectory);
	static void ChooseProjectFileAndDisplay(TSharedPtr<SEditableTextBox> TextBox, FString &DefaultOpenDirectory, FString &FileType);
	static TArray<TSharedPtr<FMapInfo>> FindMapsInProject(FString &RelativeProjectPath, bool bInMap);
};