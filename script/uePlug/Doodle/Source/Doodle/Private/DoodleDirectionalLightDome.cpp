// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleDirectionalLightDome.h"

#include "Components/DirectionalLightComponent.h"

#include "Math/UnrealMathUtility.h"
// 我自己定义的运行时曲线需要的头文件
#include "Curves/CurveFloat.h"
// 添加编辑器显示图标
#include "Components/ArrowComponent.h"
// Sets default values
ADoodleDirectionalLightDome::ADoodleDirectionalLightDome()
    : p_longitude(FRuntimeFloatCurve{}),
      p_latitude(FRuntimeFloatCurve{}),
      p_light_max(0.2),
      p_light_min(0.05),
      p_night(0.3),
      p_specular_min(0.15),
      p_castShadow(true),
      p_shadowAmout(1),
      p_specular_curve(FRuntimeFloatCurve{}),
      LightSourceAngle(10),
      LightSourceSoftAngle(10),
      LightingChannels(),
      LightColor(255, 255, 255),
      p_array_light() {
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
  auto rootComponent            = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  rootComponent->SetMobility(EComponentMobility::Stationary);
  rootComponent->SetupAttachment(RootComponent);

  SetRootComponent(rootComponent);

  // 添加一些曲线点（纬度）横线
  auto k_curve = p_latitude.GetRichCurve();
  k_curve->AutoSetTangents();
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.0, 0.75), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.25, 0.80), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.45, 0.85), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.5, 1), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.55, 0.85), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.75, 0.80), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(1, 0.75), ERichCurveTangentMode::RCTM_Auto);

  // 添加一些曲线点（经度） 竖线
  k_curve = p_longitude.GetRichCurve();
  k_curve->AutoSetTangents();
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.0, 0.75), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.25, 0.8), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.45, 0.85), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.5, 1), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.55, 0.85), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.75, 0.8), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(1, 0.75), ERichCurveTangentMode::RCTM_Auto);

  // 添加一些高光曲线
  k_curve = p_specular_curve.GetRichCurve();
  k_curve->AutoSetTangents();
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.0, 0.0), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(1, 1), ERichCurveTangentMode::RCTM_Auto);

  TArray<FVector> pos;

  const int maxSize{8};
  const double ang{360 / (double)maxSize};
  for (auto i = 0; i < maxSize; ++i) {
    for (auto j = 0; j < maxSize; ++j) {
      auto xy_ang = i * ang;
      auto z_ang  = j * ang;

      float x, y, z, w = {0};

      auto xy_rad = FMath::DegreesToRadians(xy_ang);
      auto z_rad  = FMath::DegreesToRadians(z_ang);

      FMath::SinCos(&x, &y, xy_rad);
      FMath::SinCos(&w, &z, z_rad);

      // UE_LOG(LogTemp, Log, TEXT("%f"), xy_ang);

      auto tmp_pos = FVector{x * w, y * w, z} * 300;

      auto index   = pos.FindByPredicate([=](const FVector &f) -> bool {
        return FMath::Abs((tmp_pos - f).Size()) < 3;
      });

      if (index == nullptr) {
        pos.Add(tmp_pos);
        auto name1                 = FName(FString("doodleDirection_").Append(FString::FromInt(i) + FString::FromInt(j)));
        auto tmp_light1            = CreateDefaultSubobject<UDirectionalLightComponent>(name1);
        tmp_light1->CreationMethod = EComponentCreationMethod::Native;
        tmp_light1->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        tmp_light1->SetWorldLocation(tmp_pos);
        tmp_light1->SetWorldRotation(tmp_pos.Rotation());
        tmp_pos.Rotation();

        if (tmp_pos.Z < -0.1) {
          tmp_light1->SetIntensity(0.05);
        } else {
          tmp_light1->SetIntensity(0.15);
        }

        tmp_light1->SetMobility(EComponentMobility::Stationary);
        tmp_light1->CastShadows   = p_castShadow;
        tmp_light1->SpecularScale = 1;
        tmp_light1->SetShadowAmount(p_shadowAmout);
        tmp_light1->LightSourceAngle     = LightSourceAngle;
        tmp_light1->LightSourceSoftAngle = LightSourceSoftAngle;
        tmp_light1->SetMobility(EComponentMobility::Type::Movable);
        // 设置图标 这个必须时编辑器存在时设置
#if WITH_EDITOR
        tmp_light1->StaticEditorTextureScale  = 0.0f;
        tmp_light1->StaticEditorTexture       = nullptr;
        tmp_light1->DynamicEditorTextureScale = 0.0f;
        tmp_light1->DynamicEditorTexture      = nullptr;
#endif  // WITH_EDITOR
        p_array_light.Add(tmp_light1);
      }
    }
  }

  auto x_com = CreateDefaultSubobject<UArrowComponent>("x_com");
  x_com->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
  x_com->SetArrowColor({1, 0, 0});
  x_com->SetWorldRotation({0, 0, 0});

  auto y_com = CreateDefaultSubobject<UArrowComponent>("y_com");
  y_com->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
  y_com->SetArrowColor({0, 1, 0});
  y_com->SetWorldRotation({0, 90, 0});

  auto z_com = CreateDefaultSubobject<UArrowComponent>("z_com");
  z_com->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
  z_com->SetArrowColor({0, 0, 1});
  z_com->SetWorldRotation({90, 0, 0});

  set_light();
}

#if WITH_EDITOR

void ADoodleDirectionalLightDome::PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangeEvent) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  auto name2 = PropertyChangeEvent.GetPropertyName();
  auto name  = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
  // UE_LOG(LogTemp, Log, TEXT("chick name: %s"), *(name.ToString()));
  // UE_LOG(LogTemp, Log, TEXT("chick MemberProperty: %s"), *(name2.ToString()));

  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_longitude) ||     //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_latitude) ||      //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_light_max) ||     //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_light_min) ||     //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_night) ||         //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_specular_min) ||  //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_specular_curve)) {
    // UE_LOG(LogTemp, Log, TEXT("set property: %s"), *(name.ToString()));
    set_light();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightSourceAngle)) {
    for (auto &&light : p_array_light) {
      light->LightSourceAngle = LightSourceAngle;
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightSourceSoftAngle)) {
    for (auto &&light : p_array_light) {
      light->LightSourceSoftAngle = LightSourceSoftAngle;
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightingChannels)) {
    // UE_LOG(LogTemp, Log, TEXT("set property : %s"), *(name.ToString()));
    for (auto &&light : p_array_light) {
      light->SetLightingChannels(
          LightingChannels.bChannel0,
          LightingChannels.bChannel1,
          LightingChannels.bChannel2
      );
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightColor)) {
    for (auto &&light : p_array_light) {
      light->SetLightColor(LightColor);
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_castShadow)) {
    for (auto &&light : p_array_light) {
      light->SetCastShadows(p_castShadow);
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_shadowAmout)) {
    for (auto &&light : p_array_light) {
      light->SetShadowAmount(p_shadowAmout);
    }
  }
}

#endif  // WITH_EDITOR

// Called when the game starts or when spawned
void ADoodleDirectionalLightDome::BeginPlay() {
  Super::BeginPlay();
}
void ADoodleDirectionalLightDome::set_light() {
  TArray<FVector> pos;
  const int maxSize{8};
  const double ang{360 / (double)maxSize};
  for (auto i = 0; i < maxSize; ++i) {
    for (auto j = 0; j < maxSize; ++j) {
      auto xy_ang = i * ang;
      auto z_ang  = j * ang;

      float x, y, z, w = {0};

      auto xy_rad = FMath::DegreesToRadians(xy_ang);
      auto z_rad  = FMath::DegreesToRadians(z_ang);

      FMath::SinCos(&x, &y, xy_rad);
      FMath::SinCos(&w, &z, z_rad);

      auto k_lo_z  = p_longitude.GetRichCurve()->Eval(z_ang / 360);
      auto k_la_xy = p_latitude.GetRichCurve()->Eval(xy_ang / 360);

      auto tmp_pos = FVector{x * w, y * w, z} * 300;

      auto index   = pos.FindByPredicate([=](const FVector &f) -> bool {
        return FMath::Abs((tmp_pos - f).Size()) < 1;
      });

      if (index == nullptr) {
        pos.Add(tmp_pos);
        auto name = FName(FString("doodleDirection_").Append(FString::FromInt(i) + FString::FromInt(j)));
        auto light =
            p_array_light.FindByPredicate([=](const UDirectionalLightComponent *k_l) -> bool {
              return k_l->GetFName() == name;
            });
        if (light != nullptr) {
          auto k_min     = FMath::Min(p_light_max, p_light_min);
          auto k_max     = FMath::Max(p_light_max, p_light_min);

          auto k_light_l = FMath::Max(k_min, k_la_xy * k_lo_z * k_max);
          auto k_light_n = FMath::Max(k_min, k_la_xy * k_lo_z * k_max * p_night);

          auto k_l       = ((*light)->GetComponentTransform().GetLocation().Z < -0.1) ? k_light_l : k_light_n;

          (*light)->SetIntensity(k_l);

          // UE_LOG(LogTemp, Log, TEXT("light hight: %f"), k_l);
          auto k_specular = p_specular_curve.GetRichCurve()
                                ->Eval(FMath::Max(
                                    FMath::Min(k_light_l, (float)1.0),
                                    (float)0.0
                                ));
          (*light)->SetSpecularScale((k_l > p_specular_min) ? k_specular : (float)0);
        }
      }
    }
  }
}

// Called every frame
// void ADoodleDirectionalLightDome::Tick(float DeltaTime) {
//   Super::Tick(DeltaTime);
// }
