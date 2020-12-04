// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "DoodleAbcFactory.generated.h"

class FAbcImporter;
class UAbcImportSettings;
class FAbcImporter;
class UGeometryCache;
class UAbcAssetImportData;

class SAlembicImportOptions;

/**
 * 自定义abc文件导入
 */

UCLASS()
class DOODLE_API UdoodleAbcFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()
public:

	UdoodleAbcFactory( );

	UPROPERTY( )
	UAbcImportSettings* ImportSettings;
	
	UPROPERTY( )
	bool bShowOption;

	//开始 uobj接口
	void PostInitProperties( );
	//结束接口

	//开始 ufactory 接口 
	virtual FText GetDisplayName( ) const override;
	virtual bool DoesSupportClass(UClass * Class)override;
	virtual UClass* ResolveSupportedClass( ) override;
	virtual bool FactoryCanImport(const FString& FileName) override;
	virtual UObject * FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	//结束接口

	//开始重新导入接口
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;

	void ShowImportOptionsWindow(TSharedPtr<SAlembicImportOptions>& Options, FString FilePath, const FAbcImporter& Importer);

	virtual int32 GetPriority( ) const override;
	//结束重新导入接口

	/// <summary>
	/// 导入为几何缓存
	/// </summary>
	/// <param name="Importer">import的实例</param>
	/// <param name="InParent">几何缓存的父项资产</param>
	/// <param name="Flags">为几何缓存创建的标志</param>
	/// <returns></returns>
	UObject* ImportGeometryCache(FAbcImporter& Importer, UObject* InParent, EObjectFlags Flags);

	EReimportResult::Type ReimportGeometryCache(UGeometryCache* Cache);

	void PopulateOptionsWithImportData(UAbcAssetImportData* ImportData);

};
