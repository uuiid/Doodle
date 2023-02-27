#include "DoodleCreateCharacterInstance.h"

void UDoodleCreateCharacterInstance::SetCreateCharacterConfig(const TObjectPtr<UDoodleCreateCharacterConfig>& InConfig
) {
  CurrentConfig = InConfig;
}

FAnimInstanceProxy* UDoodleCreateCharacterInstance::CreateAnimInstanceProxy() {
  return new FDoodleCreateCharacterProxy{this};
}
