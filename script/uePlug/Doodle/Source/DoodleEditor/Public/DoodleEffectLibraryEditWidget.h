#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleEffectEditorViewport.h"
#include "MovieSceneCapture.h"
#include "DoodleEffectLibraryWidget.h"
#include "AutomatedLevelSequenceCapture.h"
#include "MovieSceneCaptureDialogModule.h"

class FTypeItem;

struct FInEditorCapture1 : FMovieSceneCaptureBase
{
	FInEditorCapture1(UMovieSceneCapture* InCaptureObject, const TFunction<void(bool)>& InOnFinishedCallback)
	{
		CapturingFromWorld = nullptr;
		CaptureObject = InCaptureObject;
		OnFinishedCallback = InOnFinishedCallback;
	}

	// FMovieSceneCaptureBase interface
	virtual UWorld* GetWorld() const override
	{
		// Return a reference to the PIE world.
		return CapturingFromWorld;
	}
	virtual void OnCaptureStarted() override;
	virtual void Start() override;
	virtual void Cancel() override;
	virtual FCaptureState GetCaptureState() const override;
	// ~FMovieSceneCaptureBase interface

private:
	/** Overrides the Level Editor Play settings to specifically disable some things (such as audio playback) */
	void OverridePlaySettings(class ULevelEditorPlaySettings* PlayInEditorSettings);
	/** Called when the PIE Viewport is created. */
	void OnPIEViewportStarted();
	/** Shuts down the capture setup , called when PIE is closed by the user or the sequence finishes playing. */
	void Shutdown();
	/** Called when the user closes the PIE instance window. */
	void OnEndPIE(bool bIsSimulating);
	/** Called when the Sequence finishes playing to the end. */
	void OnLevelSequenceFinished();

private:
	UWorld* CapturingFromWorld;

	bool bScreenMessagesWereEnabled;
	float TransientPrimaryVolume;
	int32 BackedUpStreamingPoolSize;
	int32 BackedUpUseFixedPoolSize;
	int32 BackedUpTextureStreaming;
	TArray<uint8> BackedUpPlaySettings;

	bool CachedPathTracingMode = false;
	struct FEngineShowFlags* CachedEngineShowFlags = nullptr;
	TSubclassOf<AGameModeBase> CachedGameMode;
public:
	FString MapName;
	virtual void OnCaptureFinished(bool bSuccess) override;
};

struct FNewProcessCapture1 : FMovieSceneCaptureBase
{
	FNewProcessCapture1(UMovieSceneCapture* InCaptureObject, const FString& InMapNameToLoad, const TFunction<void(bool)>& InOnFinishedCallback)
	{
		CaptureObject = InCaptureObject;
		MapNameToLoad = InMapNameToLoad;
		OnFinishedCallback = InOnFinishedCallback;

		SharedProcHandle = nullptr;
	}
	// FMovieSceneCaptureBase
	virtual void Start() override;
	virtual void Cancel() override;
	virtual void OnCaptureStarted() override;
	virtual FCaptureState GetCaptureState() const override;
	// ~FMovieSceneCaptureBase

	TSharedPtr<FProcHandle> SharedProcHandle;
protected:
	FString MapNameToLoad;
public:
	virtual void OnCaptureFinished(bool bSuccess) override;
};

class FTypeItem : public TSharedFromThis<FTypeItem>
{
public:
	bool CanEdit;
	int32 TreeIndex;
	FName Name;
	FString TypePaths;

	TWeakPtr<FTypeItem> Parent;
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
	FDelegateHandle ScreenshotHandle;

	TSharedPtr<SButton> StartCaptureButton;
	TSharedPtr<STextBlock> CaptureText;
	void OnStartCapture();
	void CreateLevelSequence();
	//-----------------
	int32 PastedFrame;
	FTimerHandle TickTimer;
	void OnTickTimer();
	//TSharedPtr<FNewProcessCapture1> CurrentCapture;
	TSharedPtr<FInEditorCapture1> CurrentCapture;
	void OnCaptureFinished(bool result);
	void OnStopCapture();
	bool IsCapturing = false;
	UAutomatedLevelSequenceCapture* CaptureSeq;
	int32 StartFrame;
	int32 EndFrame;
	ULevelSequence* LevelSequence;
	//UWorld* SequenceWorld;
	UWorld* NewSequenceWorld;

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
	TObjectPtr<UObject> SelectObject;

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
	int32 MaxFrame;
	//--------------
	TWeakPtr<class SWindow> WeakCustomWindow;
	void OnResetEffect();
public:
	bool IsAutoReset;
};
