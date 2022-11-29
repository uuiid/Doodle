#include "ResizeTexture.h"
#include "Engine/Texture2D.h"
#include "ImageCore.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "FResizeTexture"

void FResizeTexture::Resize(UTexture2D* In_Texture) {
  FScopedSlowTask L_Task_Scoped{5.0f, LOCTEXT("FResizeTexture", "开始转换...")};
  L_Task_Scoped.MakeDialog();
  //   UE_LOG(LogTemp, Log, TEXT("确认调整贴图 %s"), *(In_Texture->GetPathName()));
  FTextureSource& L_Soure = In_Texture->Source;
  //   UE_LOG(LogTemp, Log, TEXT("贴图mip大小 %d"), L_Soure.GetNumMips());
  //   UE_LOG(LogTemp, Log, TEXT("贴图SizeX %d"), L_Soure.GetSizeX());
  //   UE_LOG(LogTemp, Log, TEXT("贴图SizeY %d"), L_Soure.GetSizeY());
  //   UE_LOG(LogTemp, Log, TEXT("贴图Format %d"), L_Soure.GetFormat());
  //   UE_LOG(LogTemp, Log, TEXT("贴图NumSlices %d"), L_Soure.GetNumSlices());
  //   UE_LOG(LogTemp, Log, TEXT("贴图NumBlocks %d"), L_Soure.GetNumBlocks());

  if (L_Soure.GetNumSlices() == 1 && L_Soure.GetNumBlocks() == 1 && !L_Soure.IsHDR(L_Soure.GetFormat())) {
    FImage L_Image{};
    L_Task_Scoped.EnterProgressFrame(1.0f);
    switch (L_Soure.GetFormat()) {
      case ETextureSourceFormat::TSF_G8: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::G8
        );
      } break;
      case ETextureSourceFormat::TSF_BGRA8: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::BGRA8
        );
      } break;
      case ETextureSourceFormat::TSF_BGRE8: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::BGRE8
        );
      } break;
      case ETextureSourceFormat::TSF_RGBA16: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::RGBA16
        );
      } break;
      case ETextureSourceFormat::TSF_RGBA16F: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::RGBA16F
        );
      } break;
      case ETextureSourceFormat::TSF_RGBA8: {
        UE_LOG(LogTemp, Log, TEXT("不支持的格式 %s"), (*In_Texture->GetFullName()));
        return;
      } break;
      case ETextureSourceFormat::TSF_RGBE8: {
        UE_LOG(LogTemp, Log, TEXT("不支持的格式 %s"), (*In_Texture->GetFullName()));
        return;
      } break;
      case ETextureSourceFormat::TSF_G16: {
        L_Image.Init(
            L_Soure.GetSizeX(), L_Soure.GetSizeY(), ERawImageFormat::Type::G16
        );
      } break;
      default:
        return;
        break;
    }
    L_Task_Scoped.EnterProgressFrame(1.0f);
    int32 L_Max_Size_Count = FMath::Max(FMath::CeilLogTwo(L_Soure.GetSizeX()), FMath::CeilLogTwo(L_Soure.GetSizeY()));
    int32 L_Max_Size       = FMath::Pow(2.0, L_Max_Size_Count);
    if (L_Max_Size == L_Soure.GetSizeX() && L_Max_Size == L_Soure.GetSizeY())
      return;
    /// 释放以前的显卡资源
    In_Texture->ReleaseResource();

    L_Task_Scoped.EnterProgressFrame(1.0f);
    L_Soure.GetMipData(L_Image.RawData, 0);
    // TArray64<uint8> L_Data{};

    // do {
    //   int32 L_Size = FMath::Pow(2.0, L_Max_Size_Count);
    //   UE_LOG(LogTemp, Log, TEXT("生成mip %d"), L_Max_Size_Count);
    //   FImage L_Dest;
    //   L_Image.ResizeTo(L_Dest, L_Size, L_Size, L_Image.Format, L_Image.GammaSpace);
    //   L_Data += L_Dest.RawData;

    // } while (--L_Max_Size_Count);

    // L_Soure.Init(L_Max_Size, L_Max_Size, 1, L_Max_Size_Count - 1, L_Soure.GetFormat(), L_Data.GetData());
    FImage L_Dest;
    L_Image.ResizeTo(L_Dest, L_Max_Size, L_Max_Size, L_Image.Format, L_Image.GammaSpace);
    L_Soure.Init(L_Max_Size, L_Max_Size, 1, 1, L_Soure.GetFormat(), L_Dest.RawData.GetData());
    L_Task_Scoped.EnterProgressFrame(1.0f);
    In_Texture->MipGenSettings = TextureMipGenSettings::TMGS_FromTextureGroup;

    In_Texture->MarkPackageDirty();
    In_Texture->PostEditChange();
    L_Task_Scoped.EnterProgressFrame(1.0f);
  }

  // In_Texture->Source;
}
#undef LOCTEXT_NAMESPACE