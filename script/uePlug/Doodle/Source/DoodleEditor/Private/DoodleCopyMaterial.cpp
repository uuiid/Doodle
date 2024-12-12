#include "DoodleCopyMaterial.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorAssetLibrary.h"
#include "GeometryCache.h"
#include "Materials/Material.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
// 测试使用

// 批量导入
#include "AssetRegistry/IAssetRegistry.h"
#include "DesktopPlatformModule.h"
#include "Doodle/DoodleImportFbxUI.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
// 保存包需要
#include "Editor.h"
/// 打开exe需要
#include "GenericPlatform/GenericPlatformProcess.h"
/// <summary>
/// 测试使用
/// </summary>
#include "Rendering/SkeletalMeshModel.h"
#include "MeshDescription.h"
//文件夹整理
#include "DoodleOrganizeCompoundWidget.h"
#include "DoodleEffectLibraryWidget.h"


namespace
{
	void print_test(USkeletalMesh* In_Obj)
	{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
		auto L_Lod = In_Obj->GetImportedModel();
		FSkeletalMeshImportData L_Data{};
		// In_Obj->LoadLODImportedData(0, L_Data);
		GetTargetPlatformManagerRef().GetRunningTargetPlatform();

		// FSkeletalMeshBuilder{}.Build(FSkeletalMeshBuildParameters{nullptr, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 0, true});
		FMeshDescription L_Des{};
		// if (USkeletalMeshEditorSubsystem* SkeletalMeshEditorSubsystem = GEditor->GetEditorSubsystem<USkeletalMeshEditorSubsystem>())
		// {
		// 	SkeletalMeshEditorSubsystem->GetLodBuildSettings(In_Obj, 0, OutBuildOptions);
		// }
		// L_Data.GetMeshDescription(In_Obj, L_Des);
		//for (auto &&[name, ele] : L_Des.GetElements()) {
		//  UE_LOG(LogTemp, Log, TEXT("bone name %s"), *(name.ToString()));
		//}
		for (auto&& L_Mesh : L_Lod->LODModels)
		{
			// int32 L_Num{};
			// for (auto&& L_Info : L_Mesh.ImportedMeshInfos)
			// {
			// 	L_Num += L_Info.NumVertices;
			// 	UE_LOG(LogTemp, Log, TEXT("bone name %s"), *(L_Info.Name.ToString()));
			// }
			// UE_LOG(LogTemp, Log, TEXT("FSoftSkinVertex num %d com num %d"), L_Data.Points.Num(), L_Num);
		}
#endif
	}
} // namespace

void DoodleCopyMat::Construct(const FArguments& Arg)
{
	// 这个是ue界面的创建方法
	/// clang-format off
	ChildSlot[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin(1.f, 1.f))
		[
			SNew(SButton) // 创建按钮
			.OnClicked(this, &DoodleCopyMat::getSelect) // 添加回调函数
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("获得选择物体"))) // 按钮中的字符
			]
			.ToolTipText_Lambda([=]() -> FText { return FText::FromString(TEXT("获得选中物体")); })
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin(1.f, 1.f))
		[
			SNew(SButton) // 创建按钮
			.OnClicked(this, &DoodleCopyMat::CopyMateral) // 添加回调函数
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("复制材质列表"))) // 按钮中的字符
			]
			.ToolTipText_Lambda([=]() -> FText { return FText::FromString(TEXT("复制选中物体的材质列表")); })
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin(1.f, 1.f))
		[
			SNew(SButton).OnClicked_Lambda([]() -> FReply
			{
				FGlobalTabmanager::Get()->TryInvokeTab(SDoodleImportFbxUI::Name);
				return FReply::Handled();
			}) // 批量导入
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("批量导入")))
			]
			.ToolTipText_Lambda([=]() -> FText { return FText::FromString(TEXT("批量导入fbx和abc文件")); })
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin(1.f, 1.f))
		[
			SNew(SButton).OnClicked(this, &DoodleCopyMat::BathReameAss) // 批量重命名
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("批量修改材质名称")))
			]
			.ToolTipText_Lambda([=]() -> FText
			{
				return FText::FromString(TEXT("选中骨骼物体," "会将材料名称和骨骼物体的插槽名称统一"));
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin(1.f, 1.f))
		[
			SNew(SButton).OnClicked_Lambda([this]()
			{
				FindErrorMaterials();

				return FReply::Handled();
			}) // 批量重命名
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("查找超限材质")))
			]
			.ToolTipText_Lambda([=]() -> FText
			{
				return FText::FromString(TEXT("查找贴图使用数量大于16个的材质"));
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin{1.f, 1.f})
		[
			SNew(SButton).OnClicked_Lambda([this]() -> FReply
			{
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("test")))
			]
			.ToolTipText_Lambda([]() -> FText { return FText::FromString(TEXT("测试使用")); })
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin{1.f, 1.f})
		[
			SNew(SButton).OnClicked_Lambda([this]() -> FReply
			{
				FGlobalTabmanager::Get()->TryInvokeTab(UDoodleOrganizeCompoundWidget::Name);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("整理文件夹")))
			]
			.ToolTipText_Lambda([]() -> FText { return FText::FromString(TEXT("分类整理文件夹")); })
		]
		//------------------------------
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin{1.f, 1.f})
		[
			SNew(SButton).OnClicked_Lambda([this]() -> FReply
			{
				FGlobalTabmanager::Get()->TryInvokeTab(UDoodleEffectLibraryWidget::Name);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("特效资源库")))
			]
			.ToolTipText_Lambda([]() -> FText { return FText::FromString(TEXT("分类特效资源库")); })
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).Padding(FMargin{1.f, 1.f})
		[
			SNew(SButton).OnClicked_Lambda([this]() -> FReply
			{
				FARFilter LFilter{};
				LFilter.bIncludeOnlyOnDiskAssets = false;
				LFilter.bRecursivePaths = true;
				LFilter.bRecursiveClasses = true;
				LFilter.ClassPaths.Add(UMaterialParameterCollection::StaticClass()->GetClassPathName());
				IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool
				{
					UObject* LObj =	InAss.GetAsset();
				    return true;
				});
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("修复崩溃")))
			]
			.ToolTipText_Lambda([]() -> FText { return FText::FromString(TEXT("修复5.4 材质参数集崩溃")); })
		]
	];
	/// clang-format on
}

void DoodleCopyMat::AddReferencedObjects(FReferenceCollector& collector)
{
	// collector.AddReferencedObjects();
}

FReply DoodleCopyMat::getSelect()
{
	/*
	          获得文件管理器中的骨架网格物体的选择
	          这是一个按钮的回调参数
	          */

	// 获得文件管理器的模块(或者类?)
	FContentBrowserModule& contentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> selectedAss;
	contentBrowserModle.Get().GetSelectedAssets(selectedAss);
	for (int i = 0; i < selectedAss.Num(); i++)
	{
		// 测试选中物体是否是骨骼物体
		if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>())
		{
			// 如果是骨骼物体就可以复制材质了
			UE_LOG(LogTemp, Log, TEXT("确认骨骼物体 %s"), *(selectedAss[i].GetFullName()));

			UObject* skinObj = selectedAss[i].ToSoftObjectPath().TryLoad();
			// assLoad.LoadAsset(selectedAss[i].GetFullName( ));
			// 将加载的类转换为skeletalMesh类并进行储存
			if (skinObj)
			{
				copySoureSkinObj = Cast<USkeletalMesh>(skinObj);
				UE_LOG(LogTemp, Log, TEXT("%s"), *(copySoureSkinObj->GetPathName()));
			}
		} // 测试是否是几何缓存物体
		else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass())
		{
			// 如果是骨骼物体就可以复制材质了
			UE_LOG(LogTemp, Log, TEXT("确认几何缓存  %s"), *(selectedAss[i].GetFullName()));
			UObject* cacheObj = selectedAss[i].ToSoftObjectPath().TryLoad();
			if (cacheObj)
			{
				copySoureGeoCache = cacheObj;
				//*(cacheObj->GetFullName( )
				UE_LOG(LogTemp, Log, TEXT("%s"), *(cacheObj->GetFullName()));
			}
		}
		// bool is =selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( );
		// UE_LOG(LogTemp, Log, TEXT("%s"), *(FString::FromInt(is)));
		// selectedAss[i].GetFullName( )
	}
	return FReply::Handled();
}

FReply DoodleCopyMat::CopyMateral()
{
	FContentBrowserModule& contentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> selectedAss;
	contentBrowserModle.Get().GetSelectedAssets(selectedAss);
	for (int i = 0; i < selectedAss.Num(); i++)
	{
		UObject* loadObj = selectedAss[i].ToSoftObjectPath().TryLoad(); // assLoad.LoadAsset(selectedAss[i].GetFullName( ));

		// 测试选中物体是否是骨骼物体
		if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>())
		{
			// 如果是骨骼物体就可以复制材质了
			UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName()));

			USkeletalMesh* copyTrange = Cast<USkeletalMesh>(loadObj);

			UE_LOG(LogTemp, Log, TEXT("确认并加载为几何物体 %s"), *(copyTrange->GetPathName()));

			TArray<FSkeletalMaterial> trangeMat = copyTrange->GetMaterials();

			if (copySoureSkinObj)
				// for (int m = 0; m < trangeMat.Num(); m++) {
				//   trangeMat[m] = copySoureSkinObj->GetMaterials()[m];
				//   // UE_LOG(LogTemp, Log, TEXT("%s"), *(trangeMat[m].MaterialInterface->GetPathName()));
				//   //  材质插槽命名
				// }
				copyTrange->SetMaterials(copySoureSkinObj->GetMaterials());
		} // 如果是几何缓存就复制几何缓存
		else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass())
		{
			UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName()));

			UGeometryCache* copyTrange = Cast<UGeometryCache>(loadObj);
			TArray<UMaterialInterface*> trange = copyTrange->Materials;

			if (copySoureGeoCache)
			{
				auto soure = Cast<UGeometryCache>(copySoureGeoCache);
				for (int m = 0; m < trange.Num(); m++)
				{
					trange[m] = soure->Materials[m];
					UE_LOG(LogTemp, Log, TEXT("%s"), *(trange[m]->GetPathName()));
				}
			}
			copyTrange->Materials = trange;
		}
	}
	return FReply::Handled();
}


FReply DoodleCopyMat::BathReameAss()
{
	FContentBrowserModule& contentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> selectedAss;
	contentBrowserModle.Get().GetSelectedAssets(selectedAss);

	for (auto&& item : selectedAss)
	{
		UObject* loadObj = item.GetAsset();
		if (loadObj == nullptr) continue;
		if (item.GetClass()->IsChildOf<USkeletalMesh>())
		{
			// 确认时骨骼物体
			USkeletalMesh* skinObj = Cast<USkeletalMesh>(loadObj);
			UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"), *(skinObj->GetPathName()));
			if (skinObj == nullptr) UE_LOG(LogTemp, Log, TEXT("不是骨骼物体 %s"), *(skinObj->GetPathName()));

			for (FSkeletalMaterial& mat : skinObj->GetMaterials())
			{
				if (mat.ImportedMaterialSlotName.IsValid())
				{
					set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
				}
			}
		}
		else if (item.GetClass()->IsChildOf<UStaticMesh>())
		{
			UStaticMesh* k_st = Cast<UStaticMesh>(loadObj);
			UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"), *(k_st->GetPathName()));
			if (k_st == nullptr)
			{
				UE_LOG(LogTemp, Log, TEXT("不是静态网格体 %s"), *(k_st->GetPathName()));
				continue;
			}
			for (auto& mat : k_st->GetStaticMaterials())
			{
				if (mat.ImportedMaterialSlotName.IsValid())
				{
					set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("不支持的类型"));
		}
	}
	return FReply::Handled();
}

void DoodleCopyMat::FindErrorMaterials()
{
	FARFilter LFilter{};
	LFilter.bIncludeOnlyOnDiskAssets = false;
	LFilter.bRecursivePaths = true;
	LFilter.bRecursiveClasses = true;
	LFilter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());

	TArray<UObject*> ErrorMaterials{};

	FString L_MatName{TEXT("材质 :")};
	IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool
	{
		UMaterial* L_Material = Cast<UMaterial>(InAss.GetAsset());
		if (L_Material)
		{
			TArray<FMaterialParameterInfo> L_ParameterInfo{};
			TArray<FGuid> L_ParameterIds{};
			L_Material->GetAllTextureParameterInfo(L_ParameterInfo, L_ParameterIds);
			if (L_ParameterIds.Num() > 16 && L_ParameterInfo.Num() > 16)
			{
				ErrorMaterials.Add(L_Material);
				L_MatName += L_Material->GetPathName() + TEXT(";\n");
			}
		}
		return true;
	});
	if (!ErrorMaterials.IsEmpty() && GEngine)
	{
		L_MatName += TEXT("会在渲染中出现多次编译的情况");
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(L_MatName));
		FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().SyncBrowserToAssets(ErrorMaterials);
	}
}

FReply DoodleCopyMat::set_marteral_deep()
{
	FContentBrowserModule& contentBrowserModle = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> selectedAss;
	contentBrowserModle.Get().GetSelectedAssets(selectedAss);

	// for (auto &&item : selectedAss) {
	//   UObject *loadObj = item.GetAsset();
	//   if (loadObj == nullptr)
	//     continue;
	//   if (loadObj->GetClass()->IsChildOf<UParticleSystem>()) {
	//     auto *k_par = Cast<UParticleSystem>(loadObj);
	//     for (auto *k_em : k_par->Emitters) {
	//       for (auto *k_mat_i : k_em->MeshMaterials) {
	//         auto *k_mat = k_mat_i->GetMaterial();
	//         if (k_mat) {
	//           UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
	//           k_mat->bEnableSeparateTranslucency =
	//               this->bEnableSeparateTranslucency;
	//           k_mat->ForceRecompileForRendering();
	//         }
	//       }
	//       k_em->UpdateModuleLists();
	//       for (auto *k_mod : k_em->ModulesNeedingInstanceData) {
	//         if (k_mod->GetClass()->IsChildOf<UParticleModuleRequired>()) {
	//           auto k_mod_req = Cast<UParticleModuleRequired>(k_mod);
	//           auto *k_mat    = k_mod_req->Material->GetMaterial();
	//           if (k_mat) {
	//             UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
	//             k_mat->bEnableSeparateTranslucency =
	//                 this->bEnableSeparateTranslucency;
	//             k_mat->ForceRecompileForRendering();
	//           }
	//         }
	//       }
	//     }
	//     for (auto &k_mat_s : k_par->NamedMaterialSlots) {
	//       auto *k_mat = k_mat_s.Material->GetMaterial();
	//       if (k_mat) {
	//         UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
	//         k_mat->bEnableSeparateTranslucency =
	//             this->bEnableSeparateTranslucency;
	//         k_mat->ForceRecompileForRendering();
	//       }
	//     }
	//   }
	// }

	return FReply::Handled();
}

TArray<FString> DoodleCopyMat::OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes)
{
	TArray<FString> OutFileNames;
	// FString OutDir;
	void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		uint32 SelectionFlag = 1; // A value of 0 represents single file selection while a value of 1
		// represents multiple file selection
		DesktopPlatform->OpenFileDialog(ParentWindowPtr, DialogTitle, DefaultPath, FString(""), FileTypes, SelectionFlag, OutFileNames);
		// DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
		//                                     DefaultPath, OutDir);
	}
	return OutFileNames;
}

FString DoodleCopyMat::OpenDirDialog(const FString& DialogTitle, const FString& DefaultPath)
{
	FString OutDir;
	void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle, DefaultPath, OutDir);
	}
	return OutDir;
}

void DoodleCopyMat::set_material_attr(UMaterialInterface* in_mat, const FString& in_SlotName)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	UE_LOG(LogTemp, Log, TEXT("确认材质插槽名称 %s"), *in_SlotName);
	auto old_path = in_mat->GetPathName();
	auto new_path = in_mat->GetPathName().Replace(*(in_mat->GetName()), *in_SlotName);
	if (old_path == new_path)
	{
		UE_LOG(LogTemp, Log, TEXT("材质新旧路径相同，不需要重命名"));
	}

	if (FileManager.FileExists(*new_path))
	{
		if (old_path != new_path)
		{
			UE_LOG(LogTemp, Log, TEXT("新路径存在资产,无法重命名 %s"), *(new_path));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("材质新旧路径相同，不需要重命名"));
		}
	}

	if (old_path != new_path && !FileManager.FileExists(*new_path))
	{
		UEditorAssetLibrary::RenameAsset(old_path, new_path);
		UE_LOG(LogTemp, Log, TEXT("重命名材质 路径 %s --> %s"), *(old_path), *(new_path));
	}

	if (in_mat->GetMaterial() != nullptr)
	{
		auto mat_m = in_mat->GetMaterial();
		bool l_tmp{true};
		mat_m->SetMaterialUsage(l_tmp, EMaterialUsage::MATUSAGE_GeometryCache);

		UEditorAssetLibrary::SaveAsset(mat_m->GetPathName());
		UE_LOG(LogTemp, Log, TEXT("使材料支持集合缓存 %s"), *(mat_m->GetPathName()));
	}
}

FString DoodleCopyMat::GetReferencerName() const { return TEXT("DoodleCopyMat"); }
