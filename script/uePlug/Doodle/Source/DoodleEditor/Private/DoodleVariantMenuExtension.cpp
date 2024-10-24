// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleVariantMenuExtension.h"

#include "DoodleVariantAssetUserData.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/SkinnedAssetCommon.h"

FDoodleVariantMenuExtension::FDoodleVariantMenuExtension()
{
}

FDoodleVariantMenuExtension::~FDoodleVariantMenuExtension()
{
}

void FDoodleVariantMenuExtension::AddMenuEntry(FMenuBuilder& builder) 
{
    TArray<FGuid> Objects;
    TSharedPtr<ISequencer> TempSequencer = TheSequencer.Pin();
    TempSequencer.Get()->GetSelectedObjects(Objects);
    //-----------------------------------------------
    //UMovieScene* MovieScene = TempSequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
    //const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();//Binding.GetObjectGuid()//Binding.GetTracks()
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
                    builder.BeginSection(TEXT("Doodle Variant"));
                    {
                        builder.AddSubMenu
                        (
                            FText::FromString(TEXT("切换变体")),
                            FText::FromString(TEXT("切换变体 tooltip")),
                            FNewMenuDelegate::CreateRaw(this,&FDoodleVariantMenuExtension::AddNewMenu,UserData, TempActor),
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

void FDoodleVariantMenuExtension::AddNewMenu(FMenuBuilder& builder, UDoodleVariantAssetUserData* UserData, AActor* TempActor)
{
    UDoodleVariantObject* VObject = UserData->VariantObj;
    if (VObject)
    {
        for (auto& e : VObject->AllVaraint)
        {
            builder.AddMenuEntry(
                FText::FromString(e.Key),
                FText::FromString(TEXT("Change Skeletal Mesh Variant")),
                FSlateIcon(),
                // NOTE 设置点击触发的函数
                FUIAction(FExecuteAction::CreateLambda([&, VObject, e, TempActor]()
                {
                    VObject->AllVaraint[e.Key];
                    //----------------------
                    ASkeletalMeshActor* L_Mesh = Cast<ASkeletalMeshActor>(TempActor);
                    TArray<FSkeletalMaterial> TempList = VObject->AllVaraint[e.Key].Variants;
                    for (int i = 0;i < TempList.Num();i++)
                    {
                        L_Mesh->GetSkeletalMeshComponent()->SetMaterial(i, TempList[i].MaterialInterface);
                    }
                    L_Mesh->GetSkeletalMeshComponent()->PostApplyToComponent();
                })));
        }
    }
}