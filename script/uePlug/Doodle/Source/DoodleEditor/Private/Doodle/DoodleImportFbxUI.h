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
enum class EImportSuffix : uint8 { Lig = 0, Vfx, End };

ENUM_RANGE_BY_COUNT(EImportSuffix, EImportSuffix::End)

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

	static TArray<FDoodleUSkeletonData_1> ListAllSkeletons();
};

class FSearchEpShotModel_1
{
public:
	int64 Eps{};
	int64 Shot{};
	FString ShotAb{};
	int32_t StartTime{};
	int32_t EndTime{};
	void GenStartAndEndTime(const FString& In_ImportPath);
};

class FSearchEpShotModel_2
{
public:
	int64 Eps{};
	int64 Shot{};
	FString ShotAb{};
	int32_t StartTime{};
	int32_t EndTime{};
	void GenStartAndEndTime(const FString& In_ImportPath);
};

UCLASS()
class UDoodleBaseImportData : public UObject
{
public:
	GENERATED_BODY()
	UDoodleBaseImportData()
	{
	};

	UDoodleBaseImportData(const FString& InString)
		: ImportPath(InString)
	{
	}

	virtual ~UDoodleBaseImportData()
	{
	}

	/// @brief 导入的文件路径
	FString ImportPath;

	int64 Eps{};
	int64 Shot{};
	FString ShotAb{};

	int32_t StartTime{};
	int32_t EndTime{};

	/// @brief 导入后的路径
	FString ImportPathDir{};
	/// 搜索导入的文件路径, 生成各种信息
	TVariant<FSearchEpShotModel_1, FSearchEpShotModel_2> Search_Model;

protected:
	// 生成导入的路径
	FString GetImportPath(const FString& In_Path_Prefix) const;

public:
	// @brief获取文件名称中的开始和结束, 以及集数, 镜头号等等
	void GenStartAndEndTime();
	/**
	 * @brief 根据导入的路径, 提取信息,  生成要导入ue4的路径
	 *
	 * @param In_Path 传入的路径(提取信息)
	 * @return FString  返回的导入ue4的路径
	 */
	virtual void GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix)
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

	UDoodleFbxImport_1(const FString& InString)
		: UDoodleBaseImportData(InString)
		, SkinObj()
	{
	}

	~UDoodleFbxImport_1() override
	{
	}

	/// @brief 寻找到的骨骼
	USkeleton* SkinObj;
	UAnimSequence* AnimSeq;

	USkeletalMesh* SkeletalMesh;
	UDoodleFbxCameraImport_1* CameraImport;

	bool OnlyAnim{true};
	void GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix) override;
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
	EImportSuffix Path_Suffix;
	FString Path_Prefix;

	UDoodleFbxCameraImport_1()
	{
	};

	UDoodleFbxCameraImport_1(const FString& InString)
		: UDoodleBaseImportData(InString)
	{
	}

	~UDoodleFbxCameraImport_1() override
	{
	}

	void GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix) override;
	void ImportFile() override;
	void AssembleScene() override;
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

	UDoodleAbcImport_1(const FString& InString)
		: UDoodleBaseImportData(InString)
	{
	}

	~UDoodleAbcImport_1() override
	{
	}

	void GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix) override;
	void ImportFile() override;
	void AssembleScene() override;
};

class SDoodleImportFbxUI : public SCompoundWidget, FGCObject
{
public:
	SLATE_BEGIN_ARGS(SDoodleImportFbxUI)
		{
		}

	SLATE_END_ARGS()

	using UDoodleBaseImportDataPtrType = TObjectPtr<UDoodleBaseImportData>;

	// 这里是内容创建函数
	void Construct(const FArguments& Arg);

	// 垃圾回收
	virtual void AddReferencedObjects(FReferenceCollector& collector) override;
	FString GetReferencerName() const override;

	const static FName Name;

	static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

	static FString NewFolderName;

private:
	/// @brief 导入的列表
	TSharedPtr<class SListView<UDoodleBaseImportDataPtrType>> ListImportGui;
	/// @brief 导入的列表数据
	TArray<UDoodleBaseImportDataPtrType> ListImportData;

	/// @brief 扫描的骨骼数据
	TArray<FDoodleUSkeletonData_1> AllSkinObjs;

	/// 导入路径的后缀
	FString Path_Prefix;
	EImportSuffix Path_Suffix;
	TArray<TSharedPtr<FString>> All_Path_Suffix{};
	ECheckBoxState OnlyCamera{ECheckBoxState::Unchecked};

	// 判断fbx是否是相机fbx
	bool IsCamera(const FString& In_File);
	void AddCameraFile(const FString& In_File);
	// 寻找骨骼排匹配
	void FindSK();

	// @brief 导入文件
	void ImportFile();

	/**
	 * @brief 根据传入的路径生成前缀
	 *
	 * @param In_Path_Prefix 传入的路径
	 */
	void GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix);
	// 检查fbx是否只导入动画
	void SetFbxOnlyAnim();

	// 匹配相机和文件
	void MatchCameraAndFile();

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
