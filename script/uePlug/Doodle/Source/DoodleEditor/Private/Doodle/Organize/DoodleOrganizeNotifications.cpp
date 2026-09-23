// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleOrganizeNotifications.h"

#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace DoodleOrganize
{
	namespace
	{
		void ShowNotification(const FText& Message, const TCHAR* BrushName, float FadeInDuration, const FString& ReportFile)
		{
			FNotificationInfo Info(Message);
			Info.FadeInDuration = FadeInDuration;
			Info.bUseSuccessFailIcons = false;
			Info.Image = FCoreStyle::Get().GetBrush(BrushName);

			if (!ReportFile.IsEmpty() && FPaths::FileExists(ReportFile))
			{
				Info.Hyperlink = FSimpleDelegate::CreateLambda([ReportFile]()
				{
					FPlatformProcess::LaunchFileInDefaultExternalApplication(*ReportFile);
				});
				Info.HyperlinkText = FText::FromString(TEXT("打开报告"));
			}

			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}

	void NotifyInfo(const FText& Message)
	{
		ShowNotification(Message, TEXT("MessageLog.Note"), 2.0f, FString());
	}

	void NotifySuccess(const FText& Message)
	{
		ShowNotification(Message, TEXT("MessageLog.Note"), 2.0f, FString());
	}

	void NotifyWarning(const FText& Message)
	{
		ShowNotification(Message, TEXT("MessageLog.Warning"), 2.0f, FString());
	}

	void NotifyError(const FText& Message)
	{
		ShowNotification(Message, TEXT("MessageLog.Error"), 2.0f, FString());
	}

	void NotifyReport(const FDoodleOrganizeReport& Report, const FString& ReportFile)
	{
		const FText Summary = Report.ToSummaryText();

		switch (Report.Result)
		{
		case EDoodleOrganizeResult::Success:
			ShowNotification(Summary, TEXT("MessageLog.Note"), 2.0f, ReportFile);
			break;

		case EDoodleOrganizeResult::PartialSuccess:
		case EDoodleOrganizeResult::Cancelled:
			ShowNotification(Summary, TEXT("MessageLog.Warning"), 2.0f, ReportFile);
			break;

		default:
			ShowNotification(Summary, TEXT("MessageLog.Error"), 2.0f, ReportFile);
			break;
		}

		UE_LOG(LogTemp, Log, TEXT("[DoodleOrganize] %s"), *Report.ToDetailedText());
	}

	void ShowMessage(const FText& Message)
	{
		FMessageDialog::Open(EAppMsgType::Ok, Message);
	}

	EAppReturnType::Type Confirm(const FText& Title, const FText& Message)
	{
		// UE5 的 Open 重载要 Title 传值 (传 &Title 的旧重载已废弃)
		return FMessageDialog::Open(EAppMsgType::YesNo, Message, Title);
	}
}
