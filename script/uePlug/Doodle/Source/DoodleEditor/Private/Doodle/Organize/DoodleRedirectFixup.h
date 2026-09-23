// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doodle/Organize/DoodleOrganizeTypes.h"
#include "IAssetTools.h"

/**
 * 重定向器修复。
 *
 * 与原 FixupAllReferencers() 的三点关键差别:
 *   1. 加载重定向器必须带 LOAD_NoRedirects。原实现用
 *      AssetViewUtils::LoadAssetsIfNeeded(paths, objs, /*bAllowedToPrompt* /true, /*bLoadRedirects* /true),
 *      第 4 个参数会走 LOAD_None, LoadObject 会"跟随"重定向器拿到目标对象,
 *      于是后面的 IsChildOf<UObjectRedirector>() 过滤什么都剩不下, FixupReferencers({}) 直接空转。
 *   2. 默认 LeaveFixedUpRedirectors, 不删除重定向器 —— 删除会拿掉旧路径上的引用兜底。
 *   3. 只处理调用方给出的路径, 而不是把整个 /Game 的 redirector 都捞出来。
 */
namespace DoodleOrganize
{
	DOODLEEDITOR_API FDoodleOrganizeReport FixupRedirectorsForPaths(
		const TArray<FString>& RedirectorPackagePaths,
		ERedirectFixupMode Mode = ERedirectFixupMode::LeaveFixedUpRedirectors);

	/** 扫描指定包路径下的重定向器, 返回对象路径列表 */
	DOODLEEDITOR_API TArray<FString> FindRedirectorsUnderPath(const FString& PackagePath);
}
