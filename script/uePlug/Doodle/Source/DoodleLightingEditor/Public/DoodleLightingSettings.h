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
};
