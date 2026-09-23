// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"

/**
 * 通知/弹窗的统一出口。
 * 服务层不允许直接弹窗, 只返回 FDoodleOrganizeReport; UI 层用这里的函数呈现。
 */
namespace DoodleOrganize
{
	DOODLEEDITOR_API void NotifyInfo(const FText& Message);
	DOODLEEDITOR_API void NotifySuccess(const FText& Message);
	DOODLEEDITOR_API void NotifyWarning(const FText& Message);
	DOODLEEDITOR_API void NotifyError(const FText& Message);

	/** 把报告渲染成通知; ReportFile 非空且存在时会附带「打开报告」超链接 */
	DOODLEEDITOR_API void NotifyReport(const FDoodleOrganizeReport& Report, const FString& ReportFile = FString());

	/** 只有「确定」的消息框 */
	DOODLEEDITOR_API void ShowMessage(const FText& Message);

	/** 是/否确认框 */
	DOODLEEDITOR_API EAppReturnType::Type Confirm(const FText& Title, const FText& Message);
}
