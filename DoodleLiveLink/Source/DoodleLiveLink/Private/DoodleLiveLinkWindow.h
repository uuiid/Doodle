// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** DoodleLiveLink 中心主窗口。 */
class FDoodleLiveLinkWindow
{
public:
	FDoodleLiveLinkWindow();
	~FDoodleLiveLinkWindow();

	/** 创建并显示主窗口。 */
	void CreateWindow();

private:
	/** 主窗口关闭时触发，请求退出引擎。 */
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

	TSharedPtr<class SWindow> RootWindow;
};
