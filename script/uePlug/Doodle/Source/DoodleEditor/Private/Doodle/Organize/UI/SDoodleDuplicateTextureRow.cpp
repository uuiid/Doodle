// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/UI/SDoodleDuplicateTextureRow.h"

#include "AssetThumbnail.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SExpanderArrow.h"

void SDoodleDuplicateTextureRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable,
	const TSharedPtr<FDoodleDupRowItem> InItem)
{
	Item = InItem;
	OnAssignClickedDelegate = InArgs._OnAssignClicked;
	OnRenameCommittedDelegate = InArgs._OnRenameCommitted;

	// 用 FAssetThumbnailPool 出缩略图, 不再像原实现那样每行 MakeShareable(new FSlateBrush)
	// 再 SetResourceObject(Texture) —— 那会让每行都强引用一张贴图。
	if (Item.IsValid() && !Item->bIsGroup && Item->Asset.IsValid() && InArgs._ThumbnailPool.IsValid())
	{
		Thumbnail = MakeShareable(new FAssetThumbnail(Item->Asset, 40, 40, InArgs._ThumbnailPool));
	}

	FSuperRowType::FArguments SuperArgs;
	SuperArgs.Padding(FMargin(2.0f, 2.0f));
	SMultiColumnTableRow<TSharedPtr<FDoodleDupRowItem>>::Construct(SuperArgs, InOwnerTable);
}

TSharedRef<SWidget> SDoodleDuplicateTextureRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == FName(TEXT("DoodlePath")))
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
					.IndentAmount(16)
					.ShouldDrawWires(true)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SNew(SBox)
					.WidthOverride(20.0f)
					.HeightOverride(20.0f)
					[
						// UE5 的 FAssetThumbnail 没有 MakeIcon() 了, 直接用 MakeThumbnailWidget()
						Thumbnail.IsValid()
							? Thumbnail->MakeThumbnailWidget()
							: SNullWidget::NullWidget
					]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableText)
					.IsEnabled(this, &SDoodleDuplicateTextureRow::IsEditable)
					.Text(this, &SDoodleDuplicateTextureRow::GetPathText)
					.OnTextCommitted(this, &SDoodleDuplicateTextureRow::OnPathCommitted)
			];
	}

	if (ColumnName == FName(TEXT("DoodleAction")))
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.Padding(2.0f, 0.0f)
			[
				SNew(SButton)
					.Visibility(this, &SDoodleDuplicateTextureRow::GetAssignVisibility)
					.Text(FText::FromString(TEXT("指定")))
					.ToolTipText(FText::FromString(TEXT("保留这一个, 把同组的其它副本合并到它")))
					.OnClicked(this, &SDoodleDuplicateTextureRow::OnAssignClicked)
			];
	}

	return SNullWidget::NullWidget;
}

FText SDoodleDuplicateTextureRow::GetPathText() const
{
	if (!Item.IsValid())
	{
		return FText::GetEmpty();
	}
	if (Item->bIsGroup)
	{
		return GetGroupText();
	}
	return FText::FromString(Item->DisplayPath);
}

FText SDoodleDuplicateTextureRow::GetGroupText() const
{
	if (!Item.IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(TEXT("%s   (%d 个副本)"), *Item->AssetName.ToString(), Item->Children.Num()));
}

bool SDoodleDuplicateTextureRow::IsEditable() const
{
	// 编辑态归行数据自己管 (原实现由外部戳 EditableText)
	return Item.IsValid() && !Item->bIsGroup && Item->bEditing;
}

void SDoodleDuplicateTextureRow::OnPathCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if (!Item.IsValid() || Item->bIsGroup)
	{
		return;
	}

	Item->bEditing = false;

	if (InCommitType != ETextCommit::OnEnter)
	{
		return;
	}

	const FString NewName = InText.ToString().TrimStartAndEnd();
	if (NewName.IsEmpty() || NewName.Equals(Item->Asset.AssetName.ToString(), ESearchCase::CaseSensitive))
	{
		return;
	}

	OnRenameCommittedDelegate.ExecuteIfBound(Item->Asset, Item->Asset.PackagePath.ToString(), NewName);
}

EVisibility SDoodleDuplicateTextureRow::GetAssignVisibility() const
{
	if (Item.IsValid() && !Item->bIsGroup && OnAssignClickedDelegate.IsBound())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

FReply SDoodleDuplicateTextureRow::OnAssignClicked()
{
	if (Item.IsValid())
	{
		OnAssignClickedDelegate.ExecuteIfBound(Item);
	}
	return FReply::Handled();
}
