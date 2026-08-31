// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"

/** DoodleLiveLink 中心主窗口。 */
class FDoodleLiveLinkWindow
{
public:
	FDoodleLiveLinkWindow();
	~FDoodleLiveLinkWindow();

	/** 创建并显示主窗口。 */
	void CreateWindow();

private:
	/** 创建 Live Link Face 源。 */
	void CreateLiveLinkFaceSource();

	/** 刷新源列表显示。 */
	void RefreshSourceList();

	/** 主窗口关闭时触发，请求退出引擎。 */
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

	TSharedPtr<class SWindow> RootWindow;

	/** 源列表容器。 */
	TSharedPtr<class SVerticalBox> SourceListBox;

	/** 已创建的 Live Link Face 源句柄。 */
	FLiveLinkSourceHandle LiveLinkFaceSourceHandle;
	bool bLiveLinkFaceSourceCreated = false;

	/** Live Link Face 源连接配置。 */
	FString LiveLinkFaceAddress = TEXT("127.0.0.1");
	uint16 LiveLinkFacePort = 14785;
	FString LiveLinkFaceSubjectName = TEXT("DoodleFace");
};
