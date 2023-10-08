// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantMenuExtension.h"

#include "DoodleVariantAssetUserData.h"
#include "Animation/SkeletalMeshActor.h"

DoodleVariantMenuExtension::DoodleVariantMenuExtension()
{
}

DoodleVariantMenuExtension::~DoodleVariantMenuExtension()
{
}

void DoodleVariantMenuExtension::AddMenuEntry(FMenuBuilder& builder) 
{
    TArray<FGuid> Objects;
    TSharedPtr<ISequencer> TempSequencer = TheSequencer.Pin();
    TempSequencer.Get()->GetSelectedObjects(Objects);
    //--------------------------
    if (Objects.Num() > 0)
    {
        AActor* TempActor = nullptr;
        for (TWeakObjectPtr<UObject> ptr : TempSequencer.Get()->FindObjectsInCurrentSequence(Objects[0]))
        {
            TempActor = Cast<AActor>(ptr.Get());
            if (TempActor) { break; }
        }
        if (TempActor && TempActor->IsA<ASkeletalMeshActor>() == true)
        {
            TArray<UObject*> Assets;
            TempActor->GetReferencedContentObjects(Assets);
            if (Assets.Num() > 0 && Assets[0]->GetClass()->IsChildOf<USkeletalMesh>())
            {
                USkeletalMesh* mesh = Cast<USkeletalMesh>(Assets[0]);
                UDoodleVariantAssetUserData* UserData = mesh->GetAssetUserData<UDoodleVariantAssetUserData>();
                if (UserData && UserData->VariantObj)
                {
                    builder.BeginSection("Doodle Variant");
                    {
                        builder.AddSubMenu
                        (
                            FText::FromString(TEXT("切换变体")),
                            FText::FromString(TEXT("切换变体 tooltip")),
                            FNewMenuDelegate::CreateRaw(this,&DoodleVariantMenuExtension::AddNewMenu,UserData, TempActor),
                            FUIAction(),
                            NAME_None,
                            EUserInterfaceActionType::Button,
                            false,
                            FSlateIcon()
                        );
                    }
                    builder.EndSection();
                }
            }
        }
    }
}

void DoodleVariantMenuExtension::AddNewMenu(FMenuBuilder& builder, UDoodleVariantAssetUserData* UserData, AActor* TempActor)
{
    UDoodleVariantObject* MyObject = UserData->VariantObj;
    if (MyObject)
    {
        for (auto& e : MyObject->AllVaraint)
        {
            builder.AddMenuEntry(
                FText::FromString(e.Key),
                FText::FromString(TEXT("Change Skeletal Mesh Variant")),
                FSlateIcon(),
                // NOTE 设置点击触发的函数
                FUIAction(FExecuteAction::CreateLambda([&, MyObject, e, TempActor]()
                {
                    MyObject->AllVaraint[e.Key];
                    //----------------------
                    ASkeletalMeshActor* L_Mesh = Cast<ASkeletalMeshActor>(TempActor);
                    TArray<FSkeletalMaterial> TempList = MyObject->AllVaraint[e.Key].Variants;
                    for (int i = 0;i < TempList.Num();i++)
                    {
                        L_Mesh->GetSkeletalMeshComponent()->SetMaterial(i, TempList[i].MaterialInterface);
                    }
                    L_Mesh->GetSkeletalMeshComponent()->PostApplyToComponent();
                })));
        }
    }
}