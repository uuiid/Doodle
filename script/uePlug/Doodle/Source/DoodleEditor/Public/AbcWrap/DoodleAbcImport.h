#pragma once

#include "CoreMinimal.h"
#include "Containers/List.h"
#include "Animation/MorphTarget.h"
#include "Animation/AnimSequence.h"
class UAbcDoodleImportSettings;
class UDoodleAbcAssetImportData;
namespace doodle
{
	class FAbcFile;
};

class UDoodleAlemblcCache;
class UDoodleGeometryCacheBones;

class DOODLEEDITOR_API FDoodleAbcImport
{
public:
	FDoodleAbcImport();
	~FDoodleAbcImport();
	//打开abc文件
	const bool OpenAbcFileForImport(const FString InFilePath);
	//获得结束帧
	const uint32 GetEndFrameIndex() const;
	//导入数据
	const bool ImportTrackData(const int32 InNumThreads, UAbcDoodleImportSettings *ImportSettings);
	//获得导入abc中轨道数
	const uint32 GetNumMeshTracks() const;

	UDoodleGeometryCacheBones *ImportAsDoodleGeometryCache(UObject *InParent, EObjectFlags Flags);
	UDoodleGeometryCacheBones *ReimportAsDoodleGeometryCache(UDoodleGeometryCacheBones *GeometryCache);
	void UpdateAssetImportData(UDoodleAbcAssetImportData* AssetImportData);
	void RetrieveAssetImportData(UDoodleAbcAssetImportData* ImportData);
private:
	template <typename T>
	T *CreateObjInstance(UObject *&InParent, const FString &ObjectName, const EObjectFlags Flags);
	doodle::FAbcFile *AbcFile;
	UAbcDoodleImportSettings *ImportSettings;
};