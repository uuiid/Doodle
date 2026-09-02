// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoodleLiveLinkForwarder.h"

#include "DoodleLiveLink.h"
#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "LiveLinkFaceSourceBlueprint.h"
#include "LiveLinkRole.h"

FDoodleLiveLinkForwarder::~FDoodleLiveLinkForwarder()
{
	UnregisterFrameCallback();
}

void FDoodleLiveLinkForwarder::SetFrameSink(TFunction<void(const TArray<uint8>&)> InSink)
{
	FrameSink = MoveTemp(InSink);
}

void FDoodleLiveLinkForwarder::ConnectSource(const FString& InAddress, uint16 InPort, const FString& InSubjectName)
{
	LiveLinkFaceAddress = InAddress;
	LiveLinkFacePort = InPort;
	LiveLinkFaceSubjectName = InSubjectName;

	if (!bLiveLinkFaceSourceCreated)
	{
		bool bSucceeded = false;
		ULiveLinkFaceSourceBlueprint::CreateLiveLinkFaceSource(LiveLinkFaceSourceHandle, bSucceeded);
		if (!bSucceeded)
		{
			UE_LOG(LogDoodleLiveLink, Error, TEXT("创建 Live Link Face 源失败"));
			return;
		}

		bLiveLinkFaceSourceCreated = true;
	}

	bool bConnected = false;
	ULiveLinkFaceSourceBlueprint::Connect(LiveLinkFaceSourceHandle, LiveLinkFaceSubjectName, LiveLinkFaceAddress, bConnected, LiveLinkFacePort);

	if (!bConnected)
	{
		UE_LOG(LogDoodleLiveLink, Error, TEXT("Live Link Face 源连接失败（%s:%d）"), *LiveLinkFaceAddress, LiveLinkFacePort);
		return;
	}

	UE_LOG(LogDoodleLiveLink, Log, TEXT("Live Link Face 源已连接（%s:%d, Subject=%s）"), *LiveLinkFaceAddress, LiveLinkFacePort, *LiveLinkFaceSubjectName);

	RegisterFrameCallback();
}

void FDoodleLiveLinkForwarder::RegisterFrameCallback()
{
	UnregisterFrameCallback();

	ILiveLinkClient* LiveLinkClient = nullptr;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	if (!LiveLinkClient)
	{
		return;
	}

	LiveLinkSubjectAddedHandle = LiveLinkClient->OnLiveLinkSubjectAdded().AddRaw(this, &FDoodleLiveLinkForwarder::OnLiveLinkSubjectAdded);

	// 重连时 Subject 可能已存在，立即尝试注册一次。
	TryRegisterLiveLinkFaceFrames();
}

void FDoodleLiveLinkForwarder::UnregisterFrameCallback()
{
	ILiveLinkClient* LiveLinkClient = nullptr;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	if (LiveLinkClient)
	{
		if (LiveLinkSubjectAddedHandle.IsValid())
		{
			LiveLinkClient->OnLiveLinkSubjectAdded().Remove(LiveLinkSubjectAddedHandle);
			LiveLinkSubjectAddedHandle.Reset();
		}

		if (LiveLinkFaceStaticDataHandle.IsValid() || LiveLinkFaceFrameDataHandle.IsValid())
		{
			LiveLinkClient->UnregisterSubjectFramesHandle(
				FLiveLinkSubjectName(FName(*LiveLinkFaceSubjectName)),
				LiveLinkFaceStaticDataHandle,
				LiveLinkFaceFrameDataHandle);
		}
	}

	LiveLinkFaceStaticDataHandle.Reset();
	LiveLinkFaceFrameDataHandle.Reset();
	LiveLinkFacePropertyNames.Reset();
}

void FDoodleLiveLinkForwarder::OnLiveLinkSubjectAdded(FLiveLinkSubjectKey InSubjectKey)
{
	// 仅处理与当前 Subject 名匹配的主题。
	if (InSubjectKey.SubjectName.Name.ToString() != LiveLinkFaceSubjectName)
	{
		return;
	}

	TryRegisterLiveLinkFaceFrames();
}

void FDoodleLiveLinkForwarder::TryRegisterLiveLinkFaceFrames()
{
	if (LiveLinkFaceStaticDataHandle.IsValid() || LiveLinkFaceFrameDataHandle.IsValid())
	{
		return;
	}

	ILiveLinkClient* LiveLinkClient = nullptr;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	if (!LiveLinkClient)
	{
		return;
	}

	TSubclassOf<ULiveLinkRole> SubjectRole;
	LiveLinkClient->RegisterForSubjectFrames(
		FLiveLinkSubjectName(FName(*LiveLinkFaceSubjectName)),
		FOnLiveLinkSubjectStaticDataAdded::FDelegate::CreateRaw(this, &FDoodleLiveLinkForwarder::OnLiveLinkFaceStaticData),
		FOnLiveLinkSubjectFrameDataAdded::FDelegate::CreateRaw(this, &FDoodleLiveLinkForwarder::OnLiveLinkFaceFrameData),
		LiveLinkFaceStaticDataHandle,
		LiveLinkFaceFrameDataHandle,
		SubjectRole);
}

void FDoodleLiveLinkForwarder::OnLiveLinkFaceStaticData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole,
                                                         const FLiveLinkStaticDataStruct& InStaticData)
{
	const FLiveLinkBaseStaticData* BaseData = InStaticData.GetBaseData();
	if (BaseData)
	{
		LiveLinkFacePropertyNames = BaseData->PropertyNames;
	}
}

void FDoodleLiveLinkForwarder::OnLiveLinkFaceFrameData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole,
                                                       const FLiveLinkFrameDataStruct& InFrameData)
{
	const FLiveLinkBaseFrameData* BaseData = InFrameData.GetBaseData();
	if (!BaseData || !FrameSink)
	{
		return;
	}

	// 将属性值序列化为 float 数组字节流后外送。
	TArray<uint8> Payload;
	const int32 ValueCount = BaseData->PropertyValues.Num();
	const int32 ByteCount = ValueCount * sizeof(float);
	if (ByteCount > 0)
	{
		Payload.SetNumUninitialized(ByteCount);
		FMemory::Memcpy(Payload.GetData(), BaseData->PropertyValues.GetData(), ByteCount);
	}

	FrameSink(Payload);
}
