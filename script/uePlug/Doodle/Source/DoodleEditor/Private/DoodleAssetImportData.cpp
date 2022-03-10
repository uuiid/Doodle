// Fill out your copyright notice in the Description page of Project Settings.


#include "DoodleAssetImportData.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FDoodleAssetImportData::is_valid() const
{
    return !import_file_path.IsEmpty() && !import_file_save_dir.IsEmpty();
}

#define DOODLE_TO_ATTR(Attr_name, to_fun, conv) \
   { \
        auto k_val = ImportGroupJsonData->TryGetField(#Attr_name); \
        if (k_val.IsValid())\
            Attr_name = conv(k_val->to_fun());\
   }

void FDoodleAssetImportData::initialize(TSharedPtr<FJsonObject> InImportGroupJsonData)
{
    ImportGroupJsonData = InImportGroupJsonData;

    if (ImportGroupJsonData.IsValid()) {
        DOODLE_TO_ATTR(import_file_path, AsString, );
        DOODLE_TO_ATTR(import_file_save_dir, AsString, );
        DOODLE_TO_ATTR(import_type, AsNumber, static_cast<EDoodleImportType>);
        DOODLE_TO_ATTR(fbx_skeleton_dir, AsString, );
        DOODLE_TO_ATTR(fbx_skeleton_file_name, AsString, );
        DOODLE_TO_ATTR(start_frame, AsNumber, );
        DOODLE_TO_ATTR(end_frame, AsNumber, );

    }

}