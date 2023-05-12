#include "CreateCharacterActor.h"
#include "CreateCharacterComponent.h"
ADoodleCreateCharacterActor::ADoodleCreateCharacterActor() : Super() {
  SkeletalMeshCom = CreateDefaultSubobject<UDoodleCreateCharacterComponent>(TEXT("DoodleCreateCharacterComponent"));
}