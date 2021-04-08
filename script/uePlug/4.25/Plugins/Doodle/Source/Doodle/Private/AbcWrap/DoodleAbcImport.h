#pragma once


#include "AbcImporter.h"

class UDoodleAlemblcCache;
class UDoodleGeometryCacheBones;

class DOODLE_API FDoodleAbcImport
{
public:
	FDoodleAbcImport();
	~FDoodleAbcImport();
	//打开abc文件
	const EAbcImportError OpenAbcFileForImport(const FString InFilePath);
	//获得结束帧
	const uint32 GetEndFrameIndex() const;
	//导入数据
	const EAbcImportError ImportTrackData(const int32 InNumThreads, UAbcImportSettings* ImportSettings);
	//获得导入abc中轨道数
	const uint32 GetNumMeshTracks() const;


	UDoodleGeometryCacheBones* ImportAsDoodleGeometryCache(UObject* InParent, EObjectFlags Flags);
	UDoodleGeometryCacheBones* ReimportAsDoodleGeometryCache(UDoodleGeometryCacheBones* GeometryCache);
	void UpdateAssetImportData(UAbcAssetImportData* AssetImportData);
	void RetrieveAssetImportData(UAbcAssetImportData* ImportData);
private:
	template<typename T>
	T* CreateObjInstance(UObject*& InParent, const FString& ObjectName, const EObjectFlags Flags);
	FAbcFile* AbcFile;
	UAbcImportSettings* ImportSettings;
};