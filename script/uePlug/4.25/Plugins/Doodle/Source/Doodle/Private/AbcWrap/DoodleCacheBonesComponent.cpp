#include "AbcWrap/DoodleCacheBonesComponent.h"

#include "AbcWrap/DoodleGeometryCacheBones.h"

#include "GeometryCacheComponent.h"
#include "GeometryCache.h"

UDoodleCacheBonesComponent::UDoodleCacheBonesComponent()
{
	//// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	//// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	this->p_GeometryCache_bones = /*NewObject<UGeometryCache>(this);*/
		this->CreateDefaultSubobject<UDoodleGeometryCacheBones>(FName{ UDoodleGeometryCacheBones::StaticClass()->GetName() });
}

bool UDoodleCacheBonesComponent::DoesSocketExist(FName InSocketName) const
{
	if (!p_GeometryCache_bones->p_GeometryCache_curve)
		return false;
	auto it = p_GeometryCache_bones->p_GeometryCache_curve->GetTranAnm().FindByPredicate([InSocketName](FTransformCurve& anm)->bool {
		return anm.Name.DisplayName == InSocketName;
	});
	return it != nullptr;
	//return false;
}

FTransform UDoodleCacheBonesComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	if (!p_GeometryCache_bones->p_GeometryCache_curve)
		return FTransform();

	auto tran = GetComponentTransform();
	if (InSocketName != NAME_None) {
		auto socket = p_GeometryCache_bones->p_GeometryCache_curve->GetTranAnm().FindByPredicate([InSocketName](FTransformCurve& anm)->bool {
			return anm.Name.DisplayName == InSocketName;
		});
		if (socket != nullptr) {
			TArray<float> k_time{};
			TArray<FTransform> k_tran{};

			socket->GetKeys(k_time, k_tran);
			const auto cuTime = this->p_GeometryCache_component->GetAnimationTime();
			UE_LOG(LogTemp, Log, TEXT("currtime--> %f"), cuTime);

			const auto k_t_index = k_time.IndexOfByPredicate([cuTime](float v) {
				return v > cuTime;
			});
			if (k_t_index != INDEX_NONE) {
				tran = tran * k_tran[k_t_index];
			}
			else {
				if (k_tran.Num() > 0)
					tran = tran * k_tran[0];
			}

		}
	}
	return tran;
	//return {};
}

bool UDoodleCacheBonesComponent::HasAnySockets() const
{
	if (!p_GeometryCache_component || !p_GeometryCache_bones->p_GeometryCache_curve) return false;

	UE_LOG(LogTemp, Log, TEXT("has Socket %s"), (p_GeometryCache_bones->p_GeometryCache_curve->GetTranAnm().Num() > 0) ? TEXT("true") : TEXT("fasle"));
	return p_GeometryCache_bones->p_GeometryCache_curve->GetTranAnm().Num() > 0;
	//return false;
}

void UDoodleCacheBonesComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& qutSocket) const
{
	if (!p_GeometryCache_component || !p_GeometryCache_bones->p_GeometryCache_curve) return;

	for (auto i : p_GeometryCache_bones->p_GeometryCache_curve->GetTranAnm()) {
		FComponentSocketDescription skInfo{ i.Name.DisplayName,EComponentSocketType::Socket };
		qutSocket.Add(skInfo);
	}
}


// Called when the game starts
void UDoodleCacheBonesComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void UDoodleCacheBonesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
