// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/UI/DoodleOrganizeRowItems.h"
#include "Widgets/Views/STableRow.h"

/** 路径列表 (过长路径扫描结果) 的行控件 */
class SDoodleAssetPathRow : public STableRow<TSharedPtr<FDoodlePathRowItem>>
{
public:
	SLATE_BEGIN_ARGS(SDoodleAssetPathRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable,
		const TSharedPtr<FDoodlePathRowItem> InItem);

private:
	FText GetDisplayText() const;

	TSharedPtr<FDoodlePathRowItem> Item;
};
