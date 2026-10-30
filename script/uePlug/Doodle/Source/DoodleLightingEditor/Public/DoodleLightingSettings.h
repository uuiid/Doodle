#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "DoodleLightingSettings.generated.h"

UCLASS(config = Editor, defaultconfig)
class DOODLELIGHTINGEDITOR_API UDoodleLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "本机服务端口"))
	int32 LocalPort = 0;

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "服务器 URL"))
	FString LocalServerUrl;

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "Cookies"))
	FString Cookies;

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "账号"))
	FString Account;
};
