// Fill out your copyright notice in the Description page of Project Settings.

#include "Doodle/Organize/DoodleOrganizeSettings.h"

#include "Doodle/Organize/DoodleAssetPathUtils.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "DoodleOrganize"

namespace
{
	/** 旧实现写在 GEngineIni 里的段名 */
	const TCHAR* GLegacySection = TEXT("DoodleOrganize");
}

UDoodleOrganizeSettings::UDoodleOrganizeSettings()
{
	// 构造 CDO 时只做内存迁移, 不落盘 (CDO 尚未完全初始化, SaveConfig 不安全)
	MigrateLegacyEngineIni(/*bAllowSave=*/false);
}

#if WITH_EDITOR
FName UDoodleOrganizeSettings::GetCategoryName() const
{
	// 与 UDoodleEditorSetting 保持一致, 一起出现在 Plugins 分类下
	return TEXT("Plugins");
}

FText UDoodleOrganizeSettings::GetSectionText() const
{
	return LOCTEXT("DoodleOrganizeSettingsSection", "Doodle 文件整理");
}
#endif

const UDoodleOrganizeSettings& UDoodleOrganizeSettings::Get()
{
	static bool bMigrationSaved = false;

	const UDoodleOrganizeSettings* Settings = GetDefault<UDoodleOrganizeSettings>();
	check(Settings);

	if (!bMigrationSaved)
	{
		bMigrationSaved = true;
		const_cast<UDoodleOrganizeSettings*>(Settings)->MigrateLegacyEngineIni(/*bAllowSave=*/true);
	}

	return *Settings;
}

FString UDoodleOrganizeSettings::GetTargetFolderPackagePath() const
{
	const FString FolderName = TargetFolderName.TrimStartAndEnd();
	if (FolderName.IsEmpty())
	{
		return FString();
	}
	return DoodleOrganize::CombinePackagePath(DoodleOrganize::GetGameRootPath(), FolderName);
}

FString UDoodleOrganizeSettings::GetCharacterFolderPackagePath() const
{
	const FString FolderName = CharacterFolderName.TrimStartAndEnd();
	if (FolderName.IsEmpty())
	{
		return FString();
	}
	const FString CharacterRoot = DoodleOrganize::CombinePackagePath(
		DoodleOrganize::GetGameRootPath(), TEXT("Character"));
	return DoodleOrganize::CombinePackagePath(CharacterRoot, FolderName);
}

void UDoodleOrganizeSettings::MigrateLegacyEngineIni(bool bAllowSave)
{
	if (GConfig == nullptr)
	{
		return;
	}

	bool bNeedsSave = false;

	auto MigrateString = [this, &bNeedsSave](const TCHAR* LegacyKey, FString& Target)
	{
		if (!Target.IsEmpty())
		{
			return;
		}
		FString LegacyValue;
		if (GConfig->GetString(GLegacySection, LegacyKey, LegacyValue, GEngineIni) && !LegacyValue.IsEmpty())
		{
			Target = LegacyValue;
			bNeedsSave = true;
		}
	};

	MigrateString(TEXT("TargetFolderName"), TargetFolderName);
	MigrateString(TEXT("ModeFolderName"), CharacterFolderName);
	MigrateString(TEXT("TheSuffix"), Suffix);

	if (!bNeedsSave)
	{
		return;
	}

	if (bAllowSave)
	{
		// 旧键保留不删, 便于回滚到旧版本
		SaveConfig();
	}

	UE_LOG(LogTemp, Log,
		TEXT("[DoodleOrganize] 已从 GEngineIni 的 [%s] 段迁移旧配置 (TargetFolderName / ModeFolderName / TheSuffix)"),
		GLegacySection);
}

#undef LOCTEXT_NAMESPACE
