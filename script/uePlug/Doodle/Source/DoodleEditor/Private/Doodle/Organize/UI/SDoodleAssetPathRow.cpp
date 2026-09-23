// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/UI/SDoodleAssetPathRow.h"

#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SDoodleAssetPathRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable,
	const TSharedPtr<FDoodlePathRowItem> InItem)
{
	Item = InItem;

	STableRow<TSharedPtr<FDoodlePathRowItem>>::Construct(
		STableRow<TSharedPtr<FDoodlePathRowItem>>::FArguments()
			.Padding(FMargin(2.0f, 2.0f))
			.Content()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush(TEXT("ContentBrowser.ColumnViewAssetIcon")))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(this, &SDoodleAssetPathRow::GetDisplayText)
					]
			],
		InOwnerTable);
}

FText SDoodleAssetPathRow::GetDisplayText() const
{
	if (!Item.IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(TEXT("%s   [%d]"), *Item->DisplayPath, Item->PathLength));
}
