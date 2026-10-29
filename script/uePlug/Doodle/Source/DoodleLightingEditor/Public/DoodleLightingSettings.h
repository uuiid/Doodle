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

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "服务器地址"))
	FString ServerUrl;

	UPROPERTY(config, EditAnywhere, Category = "Kitsu", meta = (DisplayName = "登录邮箱"))
	FString Email;

	UPROPERTY(config)
	FString AccessToken;

	UPROPERTY(config)
	FString RefreshToken;

	UPROPERTY(config)
	FString UserId;

	UPROPERTY(config)
	FString UserName;
};
