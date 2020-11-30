#include "doodleCopyMaterial.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorAssetLibrary.h"

#include "GeometryCache.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

void DoodleCopyMat::Construct(const FArguments & Arg)
{
    //这个是ue界面的创建方法

    ChildSlot[
        SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .HAlign(HAlign_Left)
            .Padding(FMargin(1.f,1.f))
            [
                SNew(SButton)//创建按钮
                .OnClicked(this, &DoodleCopyMat::getSelect)//添加回调函数
                [
                    SNew(STextBlock).Text(FText::FromString("Get Select Obj"))//按钮中的字符
                ]
            ]
            +SHorizontalBox::Slot( )
                .AutoWidth( )
                .HAlign(HAlign_Left)
                .Padding(FMargin(1.f, 1.f))
            [
                SNew(SButton)//创建按钮
                .OnClicked(this, &DoodleCopyMat::CopyMateral)//添加回调函数
                [
                    SNew(STextBlock).Text(FText::FromString("copy To obj"))//按钮中的字符
                ]
            ]
    ];
}

void DoodleCopyMat::AddReferencedObjects(FReferenceCollector& collector)
{
    //collector.AddReferencedObjects()
}

FReply DoodleCopyMat::getSelect( )
{
    /*
    获得文件管理器中的骨架网格物体的选择
    这是一个按钮的回调参数
    */

    //获得文件管理器的模块(或者类?)
    FContentBrowserModule& contentBrowserModle = FModuleManager::Get( ).LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FAssetData> selectedAss;
    contentBrowserModle.Get( ).GetSelectedAssets(selectedAss);
    for (int i = 0; i < selectedAss.Num( ); i++)
    {
        // 测试选中物体是否是骨骼物体
        if (selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( ))
        {
            //如果是骨骼物体就可以复制材质了
            UE_LOG(LogTemp, Log, TEXT("确认骨骼物体 %s"), *(selectedAss[i].GetFullName( )));
            
            UObject* skinObj = selectedAss[i].ToSoftObjectPath( ).TryLoad( );
            // assLoad.LoadAsset(selectedAss[i].GetFullName( ));
            //将加载的类转换为skeletalMesh类并进行储存
            if (skinObj) {
                copySoureSkinObj = Cast<USkeletalMesh>(skinObj);
                UE_LOG(LogTemp, Log, TEXT("%s"), *(copySoureSkinObj->GetPathName( )));
            }
            //TArray<FSkeletalMaterial> SoureMat = copySoureSkinObj->Materials;
            //for (int m = 0; m < SoureMat.Num( ); m++)
            //{
            //    SoureMat[m].MaterialInterface->GetPathName( );
            //    UE_LOG(LogTemp, Log, TEXT("%s"), *(SoureMat[m].MaterialInterface->GetPathName( )));
            //}
            //
            //if (UClass *cl = loadObj->GetClass())
            //{
            //    if (UProperty *mproperty = cl->FindPropertyByName("materials"))
            //    {
            //        mproperty.
            //        UE_LOG(LogTemp, Log, TEXT("%s"), *(mproperty->GetName()));
            //    }
            //}
            //selectedAss[i].ToSoftObjectPath( ).TryLoad()
            //TFieldIterator<UProperty> iter(loadObj);
            //USkeletalMeshComponent test;
            //test.getmaterial
            //test.SetMaterial( );
            //UStaticMeshComponent test2;
            //test2.SetMaterial( );
        }//测试是否是几何缓存物体
        else if (selectedAss[i].GetClass( ) == UGeometryCache::StaticClass()) {
            //如果是骨骼物体就可以复制材质了
            UE_LOG(LogTemp, Log, TEXT("确认几何缓存  %s"), *(selectedAss[i].GetFullName( )));
            UObject* cacheObj = selectedAss[i].ToSoftObjectPath( ).TryLoad( );
            if(cacheObj){
                copySoureGeoCache = cacheObj;
                //*(cacheObj->GetFullName( )
                UE_LOG(LogTemp, Log, TEXT("%s"), *(cacheObj->GetFullName( )));
            }
        }
        //bool is =selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( );
        //UE_LOG(LogTemp, Log, TEXT("%s"), *(FString::FromInt(is)));
        //selectedAss[i].GetFullName( )
    }
    return FReply::Handled( );
}

FReply DoodleCopyMat::CopyMateral( )
{
    FContentBrowserModule& contentBrowserModle = FModuleManager::Get( ).LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FAssetData> selectedAss;
    contentBrowserModle.Get( ).GetSelectedAssets(selectedAss);
    for (int i = 0; i < selectedAss.Num( ); i++)
    {
        UObject* loadObj = selectedAss[i].ToSoftObjectPath( ).TryLoad( );// assLoad.LoadAsset(selectedAss[i].GetFullName( ));

        // 测试选中物体是否是骨骼物体
        if (selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( ))
        {
            //如果是骨骼物体就可以复制材质了
            UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName( )));

            USkeletalMesh *copyTrange = Cast<USkeletalMesh>(loadObj);
            UE_LOG(LogTemp, Log, TEXT("确认并加载为几何物体 %s"), *(copyTrange->GetPathName( )));
            TArray<FSkeletalMaterial> trangeMat = copyTrange->Materials;
            if (copySoureSkinObj)
                for (int m = 0; m < trangeMat.Num( ); m++) {
                    trangeMat[m] = copySoureSkinObj->Materials[m];
                    UE_LOG(LogTemp, Log, TEXT("%s"), *(trangeMat[m].MaterialInterface->GetPathName( )));
                }
            copyTrange->Materials = trangeMat;

        }//如果是几何缓存就复制几何缓存
        else if (selectedAss[i].GetClass( ) == UGeometryCache::StaticClass( )) {
            UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName( )));

            UGeometryCache* copyTrange = Cast<UGeometryCache>(loadObj);
            TArray<UMaterialInterface *> trange = copyTrange->Materials;
            
            if (copySoureGeoCache) {
                auto soure = Cast<UGeometryCache>(copySoureGeoCache);
                for (int m = 0; m < trange.Num( ); m++)
                {
                    trange[m] = soure->Materials[m];
                    UE_LOG(LogTemp, Log, TEXT("%s"), *(trange[m]->GetPathName()));
                }
            }
            copyTrange->Materials = trange;
        }
    }
    return FReply::Handled( );
}
