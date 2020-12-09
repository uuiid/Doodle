
#include "DoodleGeometryCacheComponent.h"
#include "DoodleAlemblcCacheAsset.h"

UDoodleGeometryCacheComponent::UDoodleGeometryCacheComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

bool UDoodleGeometryCacheComponent::DoesSocketExist(FName InSocketName) const
{
	auto cache = Cast<UDoodleAlemblcCache>(GeometryCache);
	if (!cache) return false;
	return cache->tranAnm.FindByKey(InSocketName) != nullptr;
}

FTransform UDoodleGeometryCacheComponent::GetSocketTransform(FName InSocketNmae, ERelativeTransformSpace TransformSpace) const
{
	auto cache = Cast<UDoodleAlemblcCache>(GeometryCache);
	if (!cache) return FTransform();

	auto tran = GetComponentTransform();
	if (InSocketNmae != NAME_None) {
		//int32 SocketBoneIndex;
		//FTransform transform;
		//GetAnimationTime();


		auto socket = cache->tranAnm.FindByKey(InSocketNmae);
		if (socket) {
			TArray<float> k_time{};
			TArray<FTransform> k_tran{};
			socket->GetKeys(k_time, k_tran);
			const auto cuuTime = GetAnimationTime();
			//UE_LOG(LogTemp, Log, TEXT("currtime--> %f"), GetAnimationTime());

			//UE_LOG(LogTemp, Log, TEXT("tran--> %s"), *(FTransform{ {Tx,Ty,Tz,0.f},
			//													   {Rx,Ry,Rz,0.f},
			//													   {Sx,Sy,Sz,0.f} }.ToString()));

			const auto k_t_index = k_time.IndexOfByPredicate([=](float v) {return v >cuuTime; });
			if (k_t_index != INDEX_NONE) {
				tran = tran * k_tran[k_t_index];
				//UE_LOG(LogTemp, Log, TEXT("tran find--> %s"), *(k_tran[k_t_index].ToString()));
			}
			else
			{
				if (k_tran.Num() > 0)
					tran = tran * k_tran[0];
			}

		}
	}

	//switch (TransformSpace)
	//{
	//case RTS_World:
	//	//break;
	//case RTS_Actor:
	//	{
	//		if (AActor * actor = GetOwner()) {
	//			return tran.GetRelativeTransform(actor->GetTransform());
	//		}
	//	}
	//	break;
	//case RTS_Component:
	//		return tran.GetRelativeTransform(GetComponentTransform());
	//	break;
	//case RTS_ParentBoneSpace:
	//	//break;
	//default:
	//	break;
	//}
	//UE_LOG(LogTemp, Log, TEXT("tran return--> %s"), *(tran.ToString()));
	return tran;
}

bool UDoodleGeometryCacheComponent::HasAnySockets() const
{
	auto cache = Cast<UDoodleAlemblcCache>(GeometryCache);
	if (!cache) return false;
	
	UE_LOG(LogTemp, Log, TEXT("has Socket %s"),(cache->tranAnm.Num() > 0)? TEXT("true"): TEXT("fasle"));
	return cache->tranAnm.Num() > 0;
}

void UDoodleGeometryCacheComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& qutSocket) const
{
	auto cache = Cast<UDoodleAlemblcCache>(GeometryCache);
	if (!cache) return ;

	for (auto i : cache->tranAnm) {
		FComponentSocketDescription skInfo{i.Name.DisplayName,EComponentSocketType::Socket};
		qutSocket.Add(skInfo);
	}
}

void UDoodleGeometryCacheComponent::TickAtThisTime(const float Time, bool bInIsRunning, bool bInBackwards, bool bInIsLooping)
{
	Super::TickAtThisTime(Time, bInIsRunning, bInBackwards, bInIsLooping);
	UpdateChildTransforms(EUpdateTransformFlags::OnlyUpdateIfUsingSocket);
	if (GeometryCache && bRunning && !bManualTick) {
	}
}

void UDoodleGeometryCacheComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GeometryCache && bRunning && !bManualTick) {
		UpdateChildTransforms(EUpdateTransformFlags::OnlyUpdateIfUsingSocket);
	}
}
