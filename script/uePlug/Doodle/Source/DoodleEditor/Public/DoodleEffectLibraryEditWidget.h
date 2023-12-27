// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleEffectEditorViewport.h"
#include "MovieSceneCapture.h"
#include "DoodleMovieSceneCapture.h"
#include "DoodleEffectLibraryWidget.h"
/**
 * 
 */
class DOODLEEDITOR_API UDoodleEffectLibraryEditWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(UDoodleEffectLibraryEditWidget)
		{}
	SLATE_END_ARGS()

	UDoodleEffectLibraryEditWidget();

	void SetViewportDat();
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
	FString DirectoryPath = TEXT("");
	FString OutputFormat = TEXT("Effect");
	void OnSaveAndCreate();
	void OnReplaceDependencies(UObject* SObject, UObject* TObject);
	//---------------------------
	FString DescText;
	FString PreviewThumbnail;
	FString PreviewFile;
//------------------
	TSharedRef<SWidget> OnGetMenuContent();
public:
	UObject* SelectObject;

	FString EffectName;
	FString EffectType;
	FString LibraryPath;
	TArray<FString> EffectTypeValues;
};
