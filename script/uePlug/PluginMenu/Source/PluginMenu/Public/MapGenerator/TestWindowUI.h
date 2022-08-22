#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "SUserWidget.h"

#include "Reply.h"

class UTextrue2D;

class STestWindowUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STestWindowUI)
	{}
	SLATE_END_ARGS()

	TSharedPtr<class SEditableTextBox> TextFolder;
	
	void Construct(const FArguments& InArgs);
	FReply CheckReference();
	FReply RemoveActor();
	TArray<FColor> uint8ToFColor(const TArray<uint8> origin);
	TArray<FString> FindSubDirs(FString Folder);
	UTexture2D* TexFromImage(const int32 Width, const int32 Height, const TArray<FColor> &Data, const bool useAlpha);

public:
	class UTexture2D * OriginTex;
	TArray<FString> ImagePaths;
	int32 ImageIndex = 0;

	TArray<FString> FoundDirs;
	TArray<UObjectRedirector*> ObjectRedirectors;
};