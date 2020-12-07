#pragma once


#include "AbcImporter.h"

class UDoodleAlemblcCacheAsset;

class DOODLE_API FDoodleAbcImport 
{
public:
	FDoodleAbcImport();
	~FDoodleAbcImport();

	UDoodleAlemblcCacheAsset * ImportAsDoodleGeometryCache(UObject* InParent, EObjectFlags Flags);

private:
	UDoodleAlemblcCacheAsset* CreateObjInstance(UObject * & InParent, const FString & ObjectName, const EObjectFlags Flags);
	FAbcFile * AbcFile;
	UAbcImportSettings* ImportSettings;
};