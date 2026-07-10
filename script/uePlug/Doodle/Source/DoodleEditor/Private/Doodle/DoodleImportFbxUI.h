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


USTRUCT()
struct FDoodleUSkeletonData_1
{
public:
	GENERATED_BODY()
	FDoodleUSkeletonData_1()
	{
	};

	FDoodleUSkeletonData_1(const TSet<FString>& InString, USkeleton* InSkin)
		: BoneNames(InString)
		, SkinObj(InSkin)
	{
	}

	~FDoodleUSkeletonData_1()
	{
	}

	TSet<FString> BoneNames;
	USkeleton* SkinObj;
	FString SkinTag;
};

class ULevelSequence;

UCLASS()
class UDoodleBaseImportData : public UObject
{
public:
	GENERATED_BODY()
	UDoodleBaseImportData()
	{
		FrameTick = TickRate.Numerator / Rate.Numerator;
	};

	explicit UDoodleBaseImportData(const FString& InString, SDoodleImportFbxUI* In_ImportUI)
		: ImportPath(InString)
		, ImportUI(In_ImportUI)
	{
		FrameTick = TickRate.Numerator / Rate.Numerator;
	}


	/// @brief 导入的文件路径
	FString ImportPath;
	SDoodleImportFbxUI* ImportUI;

	int64 Eps{};
	int64 Shot{};
	FString ShotAb{};

	int32_t StartTime{};
	int32_t EndTime{};
	FFrameRate TickRate{60000, 1};
	FFrameRate Rate{25, 1};
	int32 FrameTick{};
	/// @brief 导入后的路径
	FString ImportPathDir{};
	static constexpr FFrameNumber Start{1001};

protected:
	// 生成导入的路径
	FString GetImportPath(const FString& In_Path_Prefix) const;

	ULevelSequence* CreateLevelSequence(const FString& InCreatePath, const FFrameNumber& In_End);

public:
	// @brief获取文件名称中的开始和结束, 以及集数, 镜头号等等
	void GenStartAndEndTime();
	/**
	 * @brief 根据导入的路径, 提取信息,  生成要导入ue4的路径
	 *
	 * @param In_Path 传入的路径(提取信息)
	 */
	virtual void GenPathPrefix()
	{
	};

	virtual void ImportFile()
	{
	};

	virtual void AssembleScene()
	{
	};

	static FString GetPathPrefix(const FString& In_Path);
};

UCLASS()
class UDoodleFbxImport_1 : public UDoodleBaseImportData
{
public:
	GENERATED_BODY()
	UDoodleFbxImport_1()
	{
	};

	UDoodleFbxImport_1(const FString& InString, SDoodleImportFbxUI* In_ImportUI)
		: UDoodleBaseImportData(InString, In_ImportUI)
		, SkinObj()
	{
	}


	/// @brief 寻找到的骨骼
	USkeleton* SkinObj;
	UAnimSequence* AnimSeq;

	USkeletalMesh* SkeletalMesh;
	UDoodleFbxCameraImport_1* CameraImport;

	bool OnlyAnim{true};
	void GenPathPrefix() override;
	void ImportFile() override;

	void AssembleScene() override;

	bool FindSkeleton(const TArray<FDoodleUSkeletonData_1> In_Skeleton);
};

UCLASS()
class UDoodleFbxCameraImport_1 : public UDoodleBaseImportData
{
public:
	GENERATED_BODY()
	// 初次导入
	bool FirstImport{false};
	FString ShotLevel_Suffix{};

	UDoodleFbxCameraImport_1()
	{
	};

	UDoodleFbxCameraImport_1(const FString& InString, SDoodleImportFbxUI* In_ImportUI)
		: UDoodleBaseImportData(InString, In_ImportUI)
	{
	}


	void GenPathPrefix() override;
	void ImportFile() override;
};

UCLASS()
class UDoodleAbcImport_1 : public UDoodleBaseImportData
{
public:
	GENERATED_BODY()

	UGeometryCache* GeometryCache;
	UDoodleFbxCameraImport_1* CameraImport;

	UDoodleAbcImport_1()
	{
	};

	UDoodleAbcImport_1(const FString& InString, SDoodleImportFbxUI* In_ImportUI)
		: UDoodleBaseImportData(InString, In_ImportUI)
	{
	}


	void GenPathPrefix() override;
	void ImportFile() override;
	void AssembleScene() override;
};

UCLASS()
class UDoodleXgenImport_1 : public UDoodleBaseImportData
{
	GENERATED_BODY()

public:
	UDoodleXgenImport_1() = default;

	explicit UDoodleXgenImport_1(const FString& InString, SDoodleImportFbxUI* In_ImportUI)
		: UDoodleBaseImportData(InString, In_ImportUI)
	{
	}

	void GenPathPrefix() override;
	void ImportFile() override;

	void AssembleScene() override;
};

// 基础的导入类
// Lig 灯光导入
// Vfx 特效导入
// WB 地编导入

USTRUCT()
struct FDoodleLevelSequenceKey
{
	GENERATED_BODY()
	FString ProjectName;
	int64 Eps{};
	int64 Shot{};
	FString ShotAb{};
	int32_t StartTime{};
	int32_t EndTime{};
};


USTRUCT()
struct FDoodleParseFileImportData : public FDoodleLevelSequenceKey
{
	GENERATED_BODY()
	bool IsCamera{};
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
	/// @brief 导入后的路径
	FString ImportPathDir{};
	static constexpr FFrameNumber Start{1001};

	ULevelSequence* CreateLevelSequence(const FString& InCreatePath, const FFrameNumber& In_End);
	void ImportFileCamera(const FDoodleListViewData& In_Path);
	void ImportFileFbx(const FDoodleListViewData& In_Path);
	void ImportFileAbc(const FDoodleListViewData& In_Path);

	virtual void CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey)
	{
	}

	virtual FString GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey)
	{
		return {};
	};

	virtual FString GetImportPath(const FDoodleParseFileImportData& In_Path) const
	{
		return {};
	}

	// 开始导入
	virtual void ImportFile(const FDoodleListViewData& In_Path)
	{
		CreateOtherData(In_Path);
	}

	// 解析文件
	FDoodleListViewData ParseFiles(const FString& InPath);
};

UCLASS()
class UDoodleLightImport : public UDoodleBaseImport
{
	GENERATED_BODY()

	FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	void ImportFile(const FDoodleListViewData& In_Path) override;
};

UCLASS()
class UDoodleVfxImport : public UDoodleBaseImport
{
	GENERATED_BODY()

	FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	void ImportFile(const FDoodleListViewData& In_Path) override;
};

UCLASS()
class UDoodleWbImport : public UDoodleBaseImport
{
	GENERATED_BODY()

	FString GetImportPath(const FDoodleParseFileImportData& In_Path) const override;
	void ImportFile(const FDoodleListViewData& In_Path) override;
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
