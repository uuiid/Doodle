#pragma once

#include "core_global.h"
#include "coresqldata.h"

CORE_NAMESPACE_S

class CORE_EXPORT assType :public coresqldata
{
public:
    assType();
    assType(const qint64 & ID_);

    void insert() override;
    void updateSQL() override;
    void deleteSQL() override;

    fileClassPtr getFile_class();
    void setFile_class(const fileClassPtrW & value);

    QString getName() const ;
    void setName(const QString & value);

private:
    QString name;
    qint64 __file_class__;

    fileClassPtrW filassP;
};

CORE_DNAMESPACE_E

