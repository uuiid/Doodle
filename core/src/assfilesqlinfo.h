#pragma once

#include "core_global.h"
#include "filesqlinfo.h"

CORE_NAMESPACE_S

class CORE_EXPORT assFileSqlInfo :public fileSqlInfo
{
public:
    assFileSqlInfo();
    assFileSqlInfo(qint64 &ID_);

    void insert() override;
    void updata() override;
    void deleteSQL() override;

    fileClassPtr getFileclass();
    void setFileClass(const fileClassPtrW& fileclass_);

    fileTypePtr getFileType();
    void setFileType(const fileTypePtrW& fileType_);

    assTypePtr getAssType();
    void setAssType(const assTypePtrW& assType_);

private:
    qint64 __file_class__;
    qint64 __file_type__;
    qint64 __ass_class__;

    fileClassPtrW fileClassP;
    fileTypePtrW fileTypeP;
    assTypePtrW assTypeP;

};

CORE_DNAMESPACE_E
