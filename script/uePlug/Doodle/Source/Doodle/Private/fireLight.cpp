// Fill out your copyright notice in the Description page of Project Settings.

#include "fireLight.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"

// Sets default values
AfireLight::AfireLight() {
  // Set this actor to call Tick() every frame.  You can turn this off to
  // improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
  RootComponent                 = CreateDefaultSubobject<USceneComponent>(TEXT("doodle_AfireLight"));
  // 添加一些默认组件
  p_LocalLight =
      CreateDefaultSubobject<UPointLightComponent>(TEXT("LocalLight"));
  p_LocalLight->SetupAttachment(RootComponent);

  // 添加一些曲线点
  p_LocalLightCurve = FRuntimeFloatCurve{};
  auto k_curve      = p_LocalLightCurve.GetRichCurve();
  k_curve->AutoSetTangents();
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.0, 0.8), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.5, 1.0), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(1.0, 0.8), ERichCurveTangentMode::RCTM_Auto);
}

// Called when the game starts or when spawned
void AfireLight::BeginPlay() { Super::BeginPlay(); }

// Called every frame
void AfireLight::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  // 获得物体创建时间
  auto time    = GetGameTimeSinceCreation();

  // 生成噪波函数
  auto noise   = FMath::PerlinNoise1D(time * Speed);
  noise        = FMath::Abs(noise);
  // FMath::DivideAndRoundNearest()

  // 获得最大和最小猪
  auto tmp_min = FMath::Min(luminanceMin, luminanceMax);
  auto tmp_max = FMath::Max(luminanceMin, luminanceMax);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
  noise = FMath::GetMappedRangeValueClamped({0.0f, 1.0f}, {tmp_min, tmp_max}, noise) *
          tmp_max;
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
  noise = FMath::GetMappedRangeValueClamped(UE::Math::TVector2{0.0f, 1.0f}, UE::Math::TVector2{tmp_min, tmp_max}, noise) *
          tmp_max;
#endif

  // 获得曲线变化
  float tmp{1.0};
  auto time_model = FMath::Modf(time, &tmp);
  noise           = p_LocalLightCurve.GetRichCurveConst()->Eval(time_model) * noise;

  // 设置灯光亮度
  if (p_LocalLight) p_LocalLight->SetIntensity(noise);
  // p_LocalLight->SetLightBrightness(noise * 1000);
  // debug宏
  // UE_LOG(LogTemp, Log, TEXT("%f"), time_modesl);
}

void AfireLight::SearchLight() {
  auto light_Transform = p_LocalLight->GetComponentTransform();

  ULocalLightComponent* light;
  if (p_LocalLight->GetClass() == UPointLightComponent::StaticClass()) {
    light = NewObject<USpotLightComponent>(this);
  } else {
    light = NewObject<UPointLightComponent>(this);
  }
  p_LocalLight->DetachFromComponent(
      FDetachmentTransformRules::KeepWorldTransform
  );
  p_LocalLight->UnregisterComponent();
  p_LocalLight->ConditionalBeginDestroy();

  light->RegisterComponent();
  light->SetWorldTransform(light_Transform);
  light->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

  // light->AttachToComponent(RootComponent,
  // FAttachmentTransformRules::KeepWorldTransform);
  p_LocalLight = light;
}

bool AfireLight::ShouldTickIfViewportsOnly() const { return true; }
