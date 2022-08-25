#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "DoodleEditorSetting.generated.h"

UCLASS(config = Editor, defaultconfig)
class UDoodleEditorSetting : public UDeveloperSettings {
  GENERATED_BODY()
 public:
  UDoodleEditorSetting();
#if WITH_EDITOR

  //~ UDeveloperSettings interface
  virtual FName GetCategoryName() const override;
  virtual FText GetSectionText() const override;
#endif
  UPROPERTY(config, EditAnywhere, Category = Doodle, meta = (DisplayName = "doodle 路径"))
  FString DoodleExePath;
};
