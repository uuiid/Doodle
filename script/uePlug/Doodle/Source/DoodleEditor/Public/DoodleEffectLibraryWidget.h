// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AssetViewTypes.h"
#include "SAssetView.h"
#include "FrontendFilters.h"

#include "Stats/StatsHierarchical.h"
#include "Styling/SlateStyle.h"
#include "MovieSceneCapture.h"
#include "Widgets/Views/STileView.h"
#include "DoodleEffectLibraryEditWidget.h"
#include "SAssetSearchBox.h"

#include "IImageWrapperModule.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Widgets/SMediaImage.h"
#include "FileMediaSource.h"
#include "IMediaEventSink.h"
#include "MediaHelpers.h"
#include "MediaSource.h"
#include "FileMediaSource.h"
#include "Components/TextBlock.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "DoodleEffectLibraryEditWidget.h"
#include "Filters/SBasicFilterBar.h"

class FTypeItem;
class FTagItem;

class DOODLEEDITOR_API FEffectTileItem
{
public:
	FName Name;
	TArray<FString> EffectTypes;
	FString TypePaths;
	TArray<FString> EffectTags;
	FString DescText;
	//--------
	FString JsonFile;
	FString PreviewFile;
	//--------------
	TSharedPtr<FSlateDynamicImageBrush> ShotBrush;
	TSharedPtr<SBox> MediaBox;
	//------------
	FDelegateHandle OnMediaEventBinding;
	TObjectPtr<UMediaPlayer> MediaPlayer;
	TObjectPtr<UMediaTexture> MediaTexture;

	FEffectTileItem()
	{
		DescText = TEXT("无");
		//------------
		MediaPlayer = NewObject<UMediaPlayer>(GetTransientPackage(), NAME_None,RF_Transient | RF_Public);
		MediaPlayer->AddToRoot();
		MediaPlayer->SetLooping(true);
		MediaPlayer->PlayOnOpen = true;
		OnMediaEventBinding = MediaPlayer->OnMediaEvent().AddRaw(this, &FEffectTileItem::HandleMediaPlayerEvent);
		//----------
		MediaTexture = NewObject<UMediaTexture>(GetTransientPackage(), NAME_None, RF_Transient | RF_Public);
		MediaTexture->AddToRoot();
		MediaTexture->SetDefaultMediaPlayer(MediaPlayer.Get());
		MediaTexture->AutoClear = true;
		MediaTexture->UpdateResource();
		MediaTexture->ClearColor = FLinearColor::Transparent;
	}

	~FEffectTileItem()
	{
		if (MediaPlayer) 
		{
			MediaPlayer->OnMediaEvent().Remove(OnMediaEventBinding);
			MediaPlayer->RemoveFromRoot();
		}
		if (MediaTexture)
			MediaTexture->RemoveFromRoot();
	}

	void SetThumbnail(FString ImagePath)
	{
		TArray64<uint8> RawFileData;
		if (FFileHelper::LoadFileToArray(RawFileData, *ImagePath))
		{
			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			//--------
			FImage Image;
			if (ImageWrapperModule.DecompressImage(RawFileData.GetData(), RawFileData.Num(), Image))
			{
				Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
				//------------------
				TArray<uint8> ImageRawData32(MoveTemp(Image.RawData));
				if (FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(*ImagePath, Image.SizeX, Image.SizeY, ImageRawData32))
				{
					ShotBrush = MakeShareable(new FSlateDynamicImageBrush(*ImagePath, FVector2D(Image.SizeX, Image.SizeY)));
				}
			}
		}
	}

	void ReadJson() 
	{
		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *JsonFile))
		{
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
			TSharedPtr<FJsonObject> JsonObject;
			FJsonSerializer::Deserialize(JsonReader, JsonObject);
			///------------
			DescText = JsonObject->GetStringField(TEXT("DescText"));
			TypePaths = JsonObject->GetStringField(TEXT("EffectType"));
			if(!TypePaths.IsEmpty())
				TypePaths.ParseIntoArray(EffectTypes, TEXT("###"),true);
			FString EffectTag = JsonObject->GetStringField(TEXT("EffectTags"));
			if(!EffectTag.IsEmpty())
				EffectTag.ParseIntoArray(EffectTags, TEXT("###"), true);
			int32 leng = EffectTags.Num();
		}
	}

	bool MatchFilter(FString TypeFilter) 
	{
		bool IsFilter = true;
		if (!TypeFilter.IsEmpty()) 
		{
			TArray<FString> TempEffectTypes;
			TypeFilter.ParseIntoArray(TempEffectTypes, TEXT("###"), true);
			if (EffectTypes.Num() < TempEffectTypes.Num()) 
			{
				IsFilter = false;
			}
			else
			{
				for (int32 I = 0; I < TempEffectTypes.Num(); I++)
				{
					if (!TempEffectTypes[I].Equals(EffectTypes[I])) 
					{
						IsFilter = false;
					}
				}
			}
		}
		return IsFilter;
	}
private:
	void HandleMediaPlayerEvent(EMediaEvent EventType)
	{
		if (EventType == EMediaEvent::PlaybackResumed)
		{
			if (MediaBox.IsValid() && MediaBox->GetVisibility() == EVisibility::Hidden)
				MediaBox->SetVisibility(EVisibility::Visible);
		}
		if (EventType == EMediaEvent::PlaybackEndReached)
		{
			//if(MediaBox)
			//	MediaBox->SetVisibility(EVisibility::Hidden);
		}
	}
};

class FTypeItemElement1 : public SMultiColumnTableRow<TSharedPtr<FTypeItem>>
{
	SLATE_BEGIN_ARGS(FTypeItemElement1) {}
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const TSharedPtr<FTypeItem> InTreeElement);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	TSharedPtr<SEditableText> TheEditableText;
private:
	TSharedPtr<FTypeItem> WeakTreeElement;
};

/** A class for check boxes in the filter list. If you double click a filter checkbox, you will enable it and disable all others */
class SFilterCheckBox : public SCheckBox
{
public:

	void SetOnFilterCtrlClicked(const FOnClicked& NewFilterCtrlClicked)
	{
		OnFilterCtrlClicked = NewFilterCtrlClicked;
	}

	void SetOnFilterAltClicked(const FOnClicked& NewFilteAltClicked)
	{
		OnFilterAltClicked = NewFilteAltClicked;
	}

	void SetOnFilterDoubleClicked(const FOnClicked& NewFilterDoubleClicked)
	{
		OnFilterDoubleClicked = NewFilterDoubleClicked;
	}

	void SetOnFilterMiddleButtonClicked(const FOnClicked& NewFilterMiddleButtonClicked)
	{
		OnFilterMiddleButtonClicked = NewFilterMiddleButtonClicked;
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnFilterDoubleClicked.IsBound())
		{
			return OnFilterDoubleClicked.Execute();
		}
		else
		{
			return SCheckBox::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
		}
	}

	virtual FReply OnMouseButtonUp(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		if (InMouseEvent.IsControlDown() && OnFilterCtrlClicked.IsBound())
		{
			return OnFilterCtrlClicked.Execute();
		}
		else if (InMouseEvent.IsAltDown() && OnFilterAltClicked.IsBound())
		{
			return OnFilterAltClicked.Execute();
		}
		else if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && OnFilterMiddleButtonClicked.IsBound())
		{
			return OnFilterMiddleButtonClicked.Execute();
		}
		else
		{
			SCheckBox::OnMouseButtonUp(InMyGeometry, InMouseEvent);
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

private:
	FOnClicked OnFilterCtrlClicked;
	FOnClicked OnFilterAltClicked;
	FOnClicked OnFilterDoubleClicked;
	FOnClicked OnFilterMiddleButtonClicked;
};

class DOODLEEDITOR_API UDoodleEffectLibraryWidget : public SCompoundWidget
{
public:
	UDoodleEffectLibraryWidget();
	~UDoodleEffectLibraryWidget();

	const static FName Name;

	SLATE_BEGIN_ARGS(UDoodleEffectLibraryWidget)
		{}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<STileView<TSharedPtr<FEffectTileItem>>> TileViewPtr;
	TArray<TSharedPtr<FEffectTileItem>> TileRootItems;
	TArray<TSharedPtr<FEffectTileItem>> AllTileRootItems;

	TSharedRef<ITableRow> MakeTableRowWidgetTile(TSharedPtr<FEffectTileItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable);
	//-----------------
	TSharedPtr<FEffectTileItem> CreateTileItem(FString Pathname);
	FString LibraryPath;
	void OnDirectoryChanged(const FString& Directory);
	FAssetData SelectAssetData;

	FString EffectType;
	FString FilterText;
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);
	void OnSearchBoxChanged(const FText& SearchText);

	void OnCreateNewEffect();
	
	TSharedPtr<FEffectTileItem> CurrentItem;

	TSharedRef<SWidget> MakeAddFilterMenu();
	//-----------------------------------
	TSharedPtr<SImage> CaptureImage;
	TSharedPtr< FSlateDynamicImageBrush > CaptureImageBrush;

	TSharedPtr<SButton> StartCaptureButton;
	bool IsCapturing = false;
	//----------------------------------
	void OnPlayPreview(TSharedPtr<FEffectTileItem> inSelectItem);
	//---------------
	void OnEffectExport();
	FString ExportDirectory;
	//----------
	TSharedPtr<SAssetSearchBox> SearchBoxPtr;
	//----------------------
	TSharedPtr<SFilterCheckBox> ToggleButtonPtr;
	TArray<FString> FilterTags;
	void OnTageCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FTagItem> InItem);
	void OnTypeSelectionChanged(TSharedPtr<FTypeItem> inSelectItem, ESelectInfo::Type SelectType);
	void OnFilterTileView();
public:
	void OnSaveNewEffect(FString EffectName);
	//-------------------
	TSharedPtr<STreeView<TSharedPtr<FTypeItem>>> TreeViewPtr;
	TArray<TSharedPtr<FTypeItem>> RootChildren;
	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FTypeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TSharedPtr<FTypeItem> InItem, TArray<TSharedPtr<FTypeItem>>& OutChildren);
	TSharedPtr<FTypeItem> NowSelectTypeItem;
	//------------
	TSharedPtr<SListView<TSharedPtr<FTagItem>>> EffectTagsViewPtr;
	TArray<TSharedPtr<FTagItem>> TheEffectTags;
	TArray<FString> AllEffectTags;
	TSharedRef<ITableRow> ListOnGenerateRow(TSharedPtr<FTagItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
};
