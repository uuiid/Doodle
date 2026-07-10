#pragma once

// clang-format off
#include "UObject/GCObject.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "DoodleImportFbxUI.generated.h"
// clang-format on

namespace UnFbx
{
	class FFbxImporter;
}

class UGeometryCache;

UENUM()
enum class EImportSuffix : uint8 { Light = 0, Vfx, WorldBinding, End };

ENUM_RANGE_BY_COUNT(EImportSuffix, EImportSuffix::End)


class SDoodleImportFbxUI;


// 基础的导入类
// Lig 灯光导入
// Vfx 特效导入
// WB 地编导入

enum class FDoodleImportType : uint8
{
	Camera,
	Fbx,
	AbcGeoCache,
	AbcHair,
	End
};

USTRUCT()
struct FDoodleLevelSequenceKey
{
	GENERATED_BODY()
	FString ProjectName;
	int32 Eps{};
	int32 Shot{};
	FString ShotAb{};
	int32_t StartTime{};
	int32_t EndTime{};

	friend bool operator==(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return Tie(Lhs.ProjectName, Lhs.Eps, Lhs.Shot, Lhs.ShotAb, Lhs.StartTime, Lhs.EndTime) == Tie(RHS.ProjectName, RHS.Eps, RHS.Shot,
			RHS.ShotAb, RHS.StartTime, RHS.EndTime);
	}

	friend bool operator!=(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend bool operator<(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return Tie(Lhs.ProjectName, Lhs.Eps, Lhs.Shot, Lhs.ShotAb, Lhs.StartTime, Lhs.EndTime) < Tie(RHS.ProjectName, RHS.Eps, RHS.Shot,
			RHS.ShotAb, RHS.StartTime, RHS.EndTime);
	}

	friend bool operator<=(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return !(RHS < Lhs);
	}

	friend bool operator>(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return RHS < Lhs;
	}

	friend bool operator>=(const FDoodleLevelSequenceKey& Lhs, const FDoodleLevelSequenceKey& RHS)
	{
		return !(Lhs < RHS);
	}
};

FORCEINLINE uint32 GetTypeHash(const FDoodleLevelSequenceKey& Key)
{
	uint32 Hash = FCrc::StrCrc32(*Key.ProjectName);
	Hash = HashCombine(Hash, GetTypeHash(Key.Eps));
	Hash = HashCombine(Hash, GetTypeHash(Key.Shot));
	Hash = HashCombine(Hash, FCrc::StrCrc32(*Key.ShotAb));
	Hash = HashCombine(Hash, GetTypeHash(Key.StartTime));
	Hash = HashCombine(Hash, GetTypeHash(Key.EndTime));
	return Hash;
}

USTRUCT()
struct FDoodleParseFileImportData : public FDoodleLevelSequenceKey
{
	GENERATED_BODY()
	FDoodleImportType IsCamera{};
};

USTRUCT()
struct FDoodleListViewData : public FDoodleParseFileImportData
{
	GENERATED_BODY()
	// 要导入的文件
	FString Path;
	// 导入后的路径
	FString ImportPath;
	FString ImportPathDir;
	UPROPERTY()
	TObjectPtr<USkeleton> Skeleton;
};

USTRUCT()
struct FDoodleBaseImportValuePair
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULevelSequence> LevelSequence;
	UPROPERTY()
	TObjectPtr<UWorld> LevelSequenceWorld;
};

UCLASS()
class UDoodleBaseImport : public UObject
{
public:
	GENERATED_BODY()
	UDoodleBaseImport()
	{
		FrameTick = TickRate.Numerator / Rate.Numerator;
	}

	UPROPERTY()
	TMap<FDoodleLevelSequenceKey, FDoodleBaseImportValuePair> LevelSequenceMap;

	FFrameRate TickRate{60000, 1};
	FFrameRate Rate{25, 1};
	int32 FrameTick{};
	static constexpr FFrameNumber Start{1001};
	SDoodleImportFbxUI* ImportUI;

	ULevelSequence* CreateLevelSequence(const FString& InCreatePath, const FFrameNumber& In_End) const;
	void ImportFileCamera(const FDoodleListViewData& In_Path);
	void ImportFileFbx(const FDoodleListViewData& In_Path);
	void ImportFileAbc(const FDoodleListViewData& In_Path);

	virtual void CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
	{
	}

	virtual FString GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
	{
		return {};
	};

	virtual FString GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
	{
		return {};
	}

	virtual FString GetImportPath(const FDoodleParseFileImportData& In_Path) const
	{
		return {};
	}

	// 开始导入
	void ImportFile(const FDoodleListViewData& In_Path);

	// 加载关卡
	void LoadLevelSequenceAndWorld(const FDoodleListViewData& In_Path);

	// 解析文件
	FDoodleListViewData ParseFiles(const FString& InPath);
};

UCLASS()
class UDoodleLightImport : public UDoodleBaseImport
{
	GENERATED_BODY()

public:
	virtual FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	virtual FString GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual FString GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual void CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
};

UCLASS()
class UDoodleVfxImport : public UDoodleBaseImport
{
	GENERATED_BODY()

public:
	virtual FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	virtual FString GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual FString GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual void CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
};

UCLASS()
class UDoodleWbImport : public UDoodleBaseImport
{
	GENERATED_BODY()

public:
	virtual FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	virtual FString GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual FString GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
	virtual void CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const override;
};


class SDoodleImportFbxUI : public SCompoundWidget, FGCObject
{
public:
	SLATE_BEGIN_ARGS(SDoodleImportFbxUI)
		{
		}

	SLATE_END_ARGS()

	using FImportDataType = TSharedPtr<FDoodleListViewData>;

	// 这里是内容创建函数
	void Construct(const FArguments& Arg);

	// 垃圾回收
	virtual void AddReferencedObjects(FReferenceCollector& collector) override;
	FString GetReferencerName() const override;

	const static FName UIName;

	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	const FString& GetUserFolderName() const;

	const FString& GetPathPrefix() const;
	const EImportSuffix& GetDepartment() const;

private:
	FString UserFolderName;
	/// @brief 导入的列表
	TSharedPtr<SListView<FImportDataType>> ListImportGui;
	/// @brief 导入的列表数据
	TArray<FImportDataType> ListImportData;
	// 导入的核心类
	TObjectPtr<UDoodleBaseImport> ImportCore;

	/// 导入路径的后缀
	EImportSuffix Department{EImportSuffix::Light};
	TArray<TSharedPtr<EImportSuffix>> All_Path_Suffix{};
	ECheckBoxState OnlyCamera{ECheckBoxState::Unchecked};

	// @brief 导入文件
	void ImportFile();


	/**
	 * @brief 添加文件
	 *
	 * @param In_Files
	 */
	void AddFile(const FString& In_File);

	// DragBegin
	///  当拖动进入一个小部件时在拖放过程中调用
	// void OnDragEnter(const FGeometry& InGeometry,  const FDragDropEvent& InDragDropEvent) override;
	/// 当拖动离开小部件时在拖放过程中调用
	// void OnDragLeave(const FDragDropEvent& InDragDropEvent) override;
	/// 当鼠标被拖动到小部件上时，在拖放过程中调用
	FReply OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override;
	/// 当用户把东西放到小部件上时被调用 终止拖放
	FReply OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override;
	// DragEnd
};
