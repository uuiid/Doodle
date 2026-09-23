// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SCompoundWidget.h"

class SDoodleOrganizePanel;

/**
 * Doodle 文件整理标签页。
 *
 * 这一层只做「标签页外壳」: 真正的界面与逻辑在 SDoodleOrganizePanel。
 * 原来的 UDoodleOrganizeCompoundWidget 既当 UObject 又当 SCompoundWidget,
 * 还要自己持有全部业务逻辑, 这里把它拆干净。
 */
class DOODLEEDITOR_API SDoodleOrganizeTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDoodleOrganizeTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * 标签页 ID。
	 * 字符串刻意保持与原 UDoodleOrganizeCompoundWidget::Name 一致,
	 * 这样已保存的编辑器布局 (Saved/Config/.../EditorLayout.ini) 不会失效。
	 */
	static const FName Name;

	/** 标签页工厂, 供 FGlobalTabmanager::RegisterNomadTabSpawner 使用 */
	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<SDoodleOrganizePanel> Panel;
};
