// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "AbcWrap/DoodleAbcImport.h"

#include "DoodleAbcFactory.generated.h"

class UAbcDoodleImportSettings;
/**
 * 这个是我们自定义的abc导入工厂
 */
UCLASS()
class DOODLEEDITOR_API UDoodleAbcFactory : public UFactory
{
	GENERATED_BODY()
public:
	UDoodleAbcFactory();

	//开始 uobj接口
	void PostInitProperties();
	//结束接口

	//开始 ufactory 接口
	virtual FText GetDisplayName() const override;
	virtual bool DoesSupportClass(UClass *Class) override;
	virtual UClass *ResolveSupportedClass() override;
	virtual bool FactoryCanImport(const FString &FileName) override;
	virtual UObject *FactoryCreateFile(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, const FString &Filename, const TCHAR *Parms, FFeedbackContext *Warn, bool &bOutOperationCanceled) override;
	//结束接口

	/// <summary>
	/// 导入为几何缓存
	/// </summary>
	/// <param name="Importer">import的实例</param>
	/// <param name="InParent">几何缓存的父项资产</param>
	/// <param name="Flags">为几何缓存创建的标志</param>
	/// <returns></returns>
	UObject *ImportGeometryCache(FDoodleAbcImport &Importer, UObject *InParent, EObjectFlags Flags);

	//EReimportResult::Type ReimportGeometryCache(UGeometryCache* Cache);

	//void PopulateOptionsWithImportData(UAbcAssetImportData* ImportData);

	UAbcDoodleImportSettings *ImportSettings;

	UPROPERTY()
	bool bShowOption;
};
