#pragma once

// clang-format off

#include "CoreMinimal.h"
#include "Exporters/Exporter.h"

#include "DoodleCustomFbxExporter.generated.h"
// clang-format on

class Doodle_CustomFbxExporter;

UCLASS()
class UDoodleCustomFbxExporter : public UExporter {
 public:
  GENERATED_BODY()

  UDoodleCustomFbxExporter();

  //~ Begin UExporter Interface
  virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags = 0) override;
  //~ End UExporter Interface

 private:
  void CreateDocument();
  void WriteToFile(const FString& In_FilePath);

  TSharedPtr<Doodle_CustomFbxExporter> Impl_Data;
};