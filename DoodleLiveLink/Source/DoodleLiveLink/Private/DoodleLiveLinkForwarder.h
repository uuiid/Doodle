// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "Templates/Function.h"

class ULiveLinkRole;

/** Live Link Face 转发器：创建/连接源，并把每帧属性值序列化后经 sink 外送。 */
class FDoodleLiveLinkForwarder
{
public:
	FDoodleLiveLinkForwarder() = default;
	~FDoodleLiveLinkForwarder();

	/** 设置帧数据输出回调（例如广播到 WebSocket）。 */
	void SetFrameSink(TFunction<void(const TArray<uint8>&)> InSink);

	/** 创建（若未创建）并连接 Live Link Face 源，随后注册帧回调。 */
	void ConnectSource(const FString& InAddress, uint16 InPort, const FString& InSubjectName);

	/** 注册 / 注销帧数据回调。 */
	void RegisterFrameCallback();
	void UnregisterFrameCallback();

private:
	/** 主题添加时，若匹配当前 Subject 名则注册帧回调。 */
	void OnLiveLinkSubjectAdded(FLiveLinkSubjectKey InSubjectKey);

	/** 尝试为当前 Subject 注册帧数据委托。 */
	void TryRegisterLiveLinkFaceFrames();

	/** 静态数据 / 帧数据回调。 */
	void OnLiveLinkFaceStaticData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole, const FLiveLinkStaticDataStruct& InStaticData);
	void OnLiveLinkFaceFrameData(FLiveLinkSubjectKey InSubjectKey, TSubclassOf<ULiveLinkRole> InSubjectRole, const FLiveLinkFrameDataStruct& InFrameData);

	TFunction<void(const TArray<uint8>&)> FrameSink;

	FDelegateHandle LiveLinkSubjectAddedHandle;
	FDelegateHandle LiveLinkFaceStaticDataHandle;
	FDelegateHandle LiveLinkFaceFrameDataHandle;
	TArray<FName> LiveLinkFacePropertyNames;

	FLiveLinkSourceHandle LiveLinkFaceSourceHandle;
	bool bLiveLinkFaceSourceCreated = false;

	FString LiveLinkFaceAddress = TEXT("127.0.0.1");
	uint16 LiveLinkFacePort = 14785;
	FString LiveLinkFaceSubjectName = TEXT("DoodleFace");
};
