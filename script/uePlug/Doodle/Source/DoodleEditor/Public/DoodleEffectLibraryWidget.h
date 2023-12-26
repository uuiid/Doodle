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

class DOODLEEDITOR_API FEffectTreeItem 
{
public:
	FName Name;
	FString EffectType;
	FString DescText;
	//--------
	FString JsonFile;
	FString PreviewFile;
	//--------------
	TSharedPtr<FSlateDynamicImageBrush> ShotBrush;
	TSharedPtr<SBox> MediaBox;
	//------------
	TObjectPtr<UMediaPlayer> MediaPlayer;
	TObjectPtr<UMediaTexture> MediaTexture;

	FEffectTreeItem()
	{
		DescText = TEXT("无");
		//------------
		MediaPlayer = NewObject<UMediaPlayer>(GetTransientPackage(), NAME_None,RF_Transient);
		MediaPlayer->AddToRoot();
		MediaPlayer->SetLooping(true);
		MediaPlayer->OnMediaEvent().AddRaw(this, &FEffectTreeItem::HandleMediaPlayerEvent);
		//----------
		MediaTexture = NewObject<UMediaTexture>(GetTransientPackage(), NAME_None, RF_Transient);
		MediaTexture->AddToRoot();
		MediaTexture->SetDefaultMediaPlayer(MediaPlayer.Get());
		MediaTexture->UpdateResource();
		MediaTexture->ClearColor = FLinearColor::Transparent;
	}

	~FEffectTreeItem()
	{
		if(MediaPlayer)
			MediaPlayer->RemoveFromRoot();
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
			EffectType = JsonObject->GetStringField(TEXT("EffectType"));
		}
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
	TSharedPtr<STileView<TSharedPtr<FEffectTreeItem>>> TileViewPtr;
	TArray<TSharedPtr<FEffectTreeItem>> TreeRootItems;
	TArray<TSharedPtr<FEffectTreeItem>> AllTreeRootItems;

	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FEffectTreeItem> InTreeElement, const TSharedRef<STableViewBase>& OwnerTable);
	//-----------------
	TSharedPtr<FEffectTreeItem> CreateTreeItem(FString Pathname);
	FString LibraryPath;
	void OnDirectoryChanged(const FString& Directory);
	FAssetData SelectAssetData;

	FString EffectType;
	FString FilterText;
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	void OnCreateNewEffect();
	
	TSharedPtr<FEffectTreeItem> CurrentItem;

	TSharedRef<SWidget> MakeAddFilterMenu();
	//-----------------------------------

	TSharedPtr<SImage> CaptureImage;
	TSharedPtr< FSlateDynamicImageBrush > CaptureImageBrush;

	TSharedPtr<SButton> StartCaptureButton;
	bool IsCapturing = false;
	//----------------------------------
	void OnPlayPreview(TSharedPtr<FEffectTreeItem> inSelectItem);
	//---------------
	void OnEffectExport();
	//FString ExportDirectory;
	//----------
	TSharedPtr<SAssetSearchBox> SearchBoxPtr;
public:
	void OnSaveNewEffect(FString EffectName);
};
