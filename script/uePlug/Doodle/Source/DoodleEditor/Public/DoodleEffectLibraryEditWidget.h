// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleEffectEditorViewport.h"
#include "MovieSceneCapture.h"
#include "DoodleMovieSceneCapture.h"
#include "DoodleEffectLibraryWidget.h"

class FTypeItem;

class FTypeItem
{
public:
	bool CanEdit;
	int32 TreeIndex;
	FName Name;
	FString TypePaths;

	TSharedPtr<FTypeItem> Parent;
	TArray<TSharedPtr<FTypeItem>> Children;

	FTypeItem();
	TSharedPtr<FTypeItem> GetChildren(FString L_Name);
	TSharedPtr<FTypeItem> AddChildren(FString L_Name);
	void ConvertToPath();
};

class FTypeItemElement : public SMultiColumnTableRow<TSharedPtr<FTypeItem>> 
{
	SLATE_BEGIN_ARGS(FTypeItemElement){}
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTypeItem> InTreeElement);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	TSharedPtr<SEditableText> TheEditableText;
private:
	TSharedPtr<FTypeItem> WeakTreeElement;
};

class FTagItem
{
public:
	FString Name{ TEXT("") };
	bool IsChecked = false;
};

class DOODLEEDITOR_API UDoodleEffectLibraryEditWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(UDoodleEffectLibraryEditWidget)
		{}
	SLATE_END_ARGS()

	UDoodleEffectLibraryEditWidget();
	~UDoodleEffectLibraryEditWidget();

	void SetAssetData(FAssetData Asset);
	void Construct(const FArguments& InArgs);
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);
	const static FName Name;
private:
	//--------Capture
	TSharedPtr<DoodleEffectEditorViewport> ViewEditorViewport;
	void OnTakeThumbnail();

	TSharedPtr<SButton> StartCaptureButton;
	TSharedPtr<STextBlock> CaptureText;
	void OnStartCapture();
	void OnCaptureFinished();
	void OnStopCapture();
	bool IsCapturing = false;
	UDoodleMovieSceneCapture* Capture;
	FString MovieExtension = TEXT("");
	TSharedPtr<SNotificationItem> NotificationItem;
	//------------------
	FString DirectoryPath;
	FString OutputFormat;
	void OnSaveAndCreate();
	void OnGetAllDependencies(UObject* SObject);
	void OnSortAssetPath(FName AssetPath);
	TArray<UObject*> AllDependens;
	TMap<UObject*, UObject*> ObjectsMap;
	//---------------------------
	FString DescText;
	FString PreviewThumbnail;
	FString PreviewFile;
//------------------
	TSharedRef<SWidget> OnGetMenuContent(TSharedPtr<FTagItem> InItem);
public:
	TSharedPtr<UObject> SelectObject;

	FString EffectName;
	FString EffectType;//##

	TSharedPtr<SListView<TSharedPtr<FTagItem>>> EffectTagsViewPtr;
	TArray<TSharedPtr<FTagItem>> EffectTags;
	FString LibraryPath;
	TArray<FString> AllEffectTags;
	TSharedRef<ITableRow> ListOnGenerateRow(TSharedPtr<FTagItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	TSharedPtr<STreeView<TSharedPtr<FTypeItem>>> TreeViewPtr;
	TArray<TSharedPtr<FTypeItem>> RootChildren;
	TSharedPtr<FTypeItem> NowSelectType;
private:
	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FTypeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TSharedPtr<FTypeItem> InItem, TArray<TSharedPtr<FTypeItem>>& OutChildren);
};
