// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Doodle/Public/doodleAbcFactory.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodedoodleAbcFactory() {}
// Cross Module References
	DOODLE_API UClass* Z_Construct_UClass_UdoodleAbcFactory_NoRegister();
	DOODLE_API UClass* Z_Construct_UClass_UdoodleAbcFactory();
	UNREALED_API UClass* Z_Construct_UClass_UFactory();
	UPackage* Z_Construct_UPackage__Script_doodle();
	ALEMBICLIBRARY_API UClass* Z_Construct_UClass_UAbcImportSettings_NoRegister();
// End Cross Module References
	void UdoodleAbcFactory::StaticRegisterNativesUdoodleAbcFactory()
	{
	}
	UClass* Z_Construct_UClass_UdoodleAbcFactory_NoRegister()
	{
		return UdoodleAbcFactory::StaticClass();
	}
	struct Z_Construct_UClass_UdoodleAbcFactory_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_bShowOption_MetaData[];
#endif
		static void NewProp_bShowOption_SetBit(void* Obj);
		static const UE4CodeGen_Private::FBoolPropertyParams NewProp_bShowOption;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_ImportSettings_MetaData[];
#endif
		static const UE4CodeGen_Private::FObjectPropertyParams NewProp_ImportSettings;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UdoodleAbcFactory_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UFactory,
		(UObject* (*)())Z_Construct_UPackage__Script_doodle,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UdoodleAbcFactory_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * ?\xd4\xb6???abc?\xc4\xbc?????\n */" },
		{ "IncludePath", "doodleAbcFactory.h" },
		{ "ModuleRelativePath", "Public/doodleAbcFactory.h" },
		{ "ToolTip", "?\xd4\xb6???abc?\xc4\xbc?????" },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption_MetaData[] = {
		{ "ModuleRelativePath", "Public/doodleAbcFactory.h" },
	};
#endif
	void Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption_SetBit(void* Obj)
	{
		((UdoodleAbcFactory*)Obj)->bShowOption = 1;
	}
	const UE4CodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption = { "bShowOption", nullptr, (EPropertyFlags)0x0010000000000000, UE4CodeGen_Private::EPropertyGenFlags::Bool | UE4CodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, sizeof(bool), sizeof(UdoodleAbcFactory), &Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption_SetBit, METADATA_PARAMS(Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_ImportSettings_MetaData[] = {
		{ "ModuleRelativePath", "Public/doodleAbcFactory.h" },
	};
#endif
	const UE4CodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_ImportSettings = { "ImportSettings", nullptr, (EPropertyFlags)0x0010000000000000, UE4CodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(UdoodleAbcFactory, ImportSettings), Z_Construct_UClass_UAbcImportSettings_NoRegister, METADATA_PARAMS(Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_ImportSettings_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_ImportSettings_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UdoodleAbcFactory_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_bShowOption,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UdoodleAbcFactory_Statics::NewProp_ImportSettings,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UdoodleAbcFactory_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UdoodleAbcFactory>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UdoodleAbcFactory_Statics::ClassParams = {
		&UdoodleAbcFactory::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UdoodleAbcFactory_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UdoodleAbcFactory_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UdoodleAbcFactory_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UdoodleAbcFactory_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UdoodleAbcFactory()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UdoodleAbcFactory_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UdoodleAbcFactory, 2115545815);
	template<> DOODLE_API UClass* StaticClass<UdoodleAbcFactory>()
	{
		return UdoodleAbcFactory::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_UdoodleAbcFactory(Z_Construct_UClass_UdoodleAbcFactory, &UdoodleAbcFactory::StaticClass, TEXT("/Script/doodle"), TEXT("UdoodleAbcFactory"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UdoodleAbcFactory);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
