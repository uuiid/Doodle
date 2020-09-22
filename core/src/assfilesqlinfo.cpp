#include "assfilesqlinfo.h"
#include "coreset.h"
#include "fileclass.h"
#include "filetype.h"
#include "asstype.h"

#include "sql_builder/sql.h"

#include <QVariant>
#include <QSqlError>

CORE_NAMESPACE_S

assFileSqlInfo::assFileSqlInfo()
{
    __file_class__ = -1;
    __file_type__ = -1;
    __ass_class__ = -1;
}

assFileSqlInfo::assFileSqlInfo(qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__file_class__", "__file_type__", "__ass_class__");
    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        return;
    if (query->next())
    {
        idP = query->value(0).toInt();
        fileP = query->value(1).toString();
        fileSuffixesP = query->value(2).toString();
        userP = query->value(3).toString();
        versionP = query->value(4).toInt();
        filepathP = query->value(5).toString();
        infoP = query->value(6).toString();
        fileStateP = query->value(7).toString();
        __file_class__ = query->value(8).toInt();
        __file_type__ = query->value(9).toInt();
        __ass_class__ = query->value(10).toInt();
        return;
    }
    //失败保护
    idP = -1;
}

void assFileSqlInfo::insert()
{
    sql::InsertModel ins_;
    if (idP < 0)
    {
        sqlQuertPtr query = coreSql::getCoreSql().getquery();
        ins_.insert("file", fileP.toStdString());
        ins_.insert("fileSuffixes", fileSuffixesP.toStdString());
        ins_.insert("user", userP.toStdString());
        ins_.insert("version", (unsigned char)versionP);
        ins_.insert("_file_path_", filepathP.toStdString());

        if (!infoP.isEmpty())
            ins_.insert("infor", infoP.toStdString());

        if (!fileStateP.isEmpty())
            ins_.insert("filestate", fileStateP.toStdString());

        if (__file_class__ > 0)
            ins_.insert("__file_class__", (unsigned char)__file_class__);
        if (__file_type__ > 0)
            ins_.insert("__file_type__", (unsigned char)__file_type__);
        if (__ass_class__ > 0)
            ins_.insert("__ass_class__", (unsigned char)__ass_class__);
        ins_.into(coreSet::getCoreSet().getProjectname().toStdString() + ".basefile");
        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);
        query->finish();
    }
}

void assFileSqlInfo::updateSQL()
{
    sql::UpdateModel updatasql_;
    updatasql_.update(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    updatasql_.set("filestate", fileSuffixesP.toStdString());
    updatasql_.set("infor", infoP.toStdString());
    updatasql_.set("__file_class__", __file_class__);
    updatasql_.set("__file_type__", __file_type__);
    updatasql_.set("__ass_class__", __ass_class__);

    updatasql_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(updatasql_.str())))
        throw std::runtime_error("not updata fileinfo");
    query->finish();
}

void assFileSqlInfo::deleteSQL()
{
}

fileClassPtr assFileSqlInfo::getFileclass()
{
    if (fileClassP != nullptr)
    {
        return fileClassP;
    }
    else
    {
        fileClassPtrW p_ = fileClassPtr(new fileClass(__file_class__));
        fileClassP = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setFileClass(const fileClassPtrW &fileclass_)
{
    fileClassP = fileclass_;
    __file_class__ = fileclass_.lock()->getIdP();
}

fileTypePtr assFileSqlInfo::getFileType()
{
    if (fileTypeP != nullptr)
    {
        return fileTypeP;
    }
    else
    {
        fileTypePtr p_ = fileTypePtr(new fileType(__file_type__));
        fileTypeP = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setFileType(const fileTypePtrW &fileType_)
{
    fileTypeP = fileType_;
    __file_type__ = fileType_.lock()->getIdP();
}

assTypePtr assFileSqlInfo::getAssType()
{
    if (assTypeP != nullptr)
    {
        return assTypeP;
    }
    else
    {
        assTypePtr p_ = assTypePtr(new assType(__ass_class__));
        assTypeP = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setAssType(const assTypePtrW &assType_)
{
    assTypeP = assType_;
    __ass_class__ = assType_.lock()->getIdP();
}

CORE_DNAMESPACE_E
