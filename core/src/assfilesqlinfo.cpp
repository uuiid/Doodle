#include "assfilesqlinfo.h"
#include "coreset.h"
#include "fileclass.h"
#include "filetype.h"
#include "asstype.h"

#include "sql_builder/sql.h"

#include <QVariant>
#include <QSqlError>

#include <iostream>
CORE_NAMESPACE_S

assFileSqlInfo::assFileSqlInfo()
{
    __file_class__ = -1;
    __file_type__ = -1;
    __ass_class__ = -1;

    p_ptrW_fileClass = nullptr;
    p_ptrW_fileType = nullptr;
    p_ptrW_assType = nullptr;
}

void assFileSqlInfo::select(qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__file_class__", "__file_type__", "__ass_class__");
    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());
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

        if (!query->value(8).isNull())
            __file_class__ = query->value(8).toInt();

        if (!query->value(9).isNull())
            __file_type__ = query->value(9).toInt();

        if (!query->value(10).isNull())
            __ass_class__ = query->value(10).toInt();

        return;
    }
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

    if (__file_class__ >= 0)
        updatasql_.set("__file_class__", __file_class__);
    if (__file_type__ >= 0)
        updatasql_.set("__file_type__", __file_type__);
    if (__ass_class__ >= 0)
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

assInfoPtrList assFileSqlInfo::batchQuerySelect(sqlQuertPtr &query)
{
    assInfoPtrList list;
    while (query->next())
    {
        assInfoPtr ass_(new assFileSqlInfo);
        ass_->idP = query->value(0).toInt();
        ass_->fileP = query->value(1).toString();
        ass_->fileSuffixesP = query->value(2).toString();
        ass_->userP = query->value(3).toString();
        ass_->versionP = query->value(4).toInt();
        ass_->filepathP = query->value(5).toString();
        ass_->infoP = query->value(6).toString();
        ass_->fileStateP = query->value(7).toString();

        if (!query->value(8).isNull())
            ass_->__file_class__ = query->value(8).toInt();

        if (!query->value(9).isNull())
            ass_->__file_type__ = query->value(9).toInt();

        if (!query->value(10).isNull())
            ass_->__ass_class__ = query->value(10).toInt();

        list.append(ass_);
    }
    return list;
}

assInfoPtrList assFileSqlInfo::getAll(const fileClassPtr &fc_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__file_class__", "__file_type__", "__ass_class__");
    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_class__") == fc_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());

    assInfoPtrList list = batchQuerySelect(query);

    for (auto &x : list)
    {
        x->setFileClass(fc_);
    }
    return list;
}

assInfoPtrList assFileSqlInfo::getAll(const fileTypePtr &ft_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__file_class__", "__file_type__", "__ass_class__");
    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_type__") == ft_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());

    assInfoPtrList list = batchQuerySelect(query);

    for (auto &x : list)
    {
        x->setFileType(ft_);
    }
    return list;
}

assInfoPtrList assFileSqlInfo::getAll(const assTypePtr &AT_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__file_class__", "__file_type__", "__ass_class__");
    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__ass_class__") == AT_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error(query->lastError().text().toStdString());

    assInfoPtrList list = batchQuerySelect(query);

    for (auto &x : list)
    {
        x->setAssType(AT_);
    }
    return list;
}
QFileInfo assFileSqlInfo::generatePath(const QString &programFolder)
{
    QString path("%1/%2/%3/%4/%5");

    //第一次 格式化添加根目录
    path = path.arg(coreSet::getCoreSet().getAssRoot().absolutePath());

    //第二次添加类型
    fileClassPtr fc_ = getFileClass();
    if (fc_)
        path = path.arg(fc_->getFileclass_str());
    else
        path = path.arg(QString());

    //第三次格式化添加  ass_type
    assTypePtr at_ = getAssType();
    if (at_)
        path = path.arg(at_->getAssType());
    else
        path = path.arg(QString());

    //第五次格式化程序文件夹
    path = path.arg(programFolder);

    //第五次添加fileType
    fileTypePtr ft_ = getFileType();
    if (ft_)
        path = path.arg(ft_->getFileType());
    else
        path = path.arg(QString());

    return path;
}

QFileInfo assFileSqlInfo::generatePath(const QString &programFolder, const QString &suffixes)
{
    QString path("%1/%2");
    path = path.arg(generatePath(programFolder).filePath());
    path = path.arg(generateFileName(suffixes));
    return path;
}

QFileInfo assFileSqlInfo::generatePath(const QString &programFolder, const QString &suffixes, const QString &prefix)
{
    QString path("%1/%2");
    path = path.arg(generatePath(programFolder).filePath());
    path = path.arg(generateFileName(suffixes, prefix));
    return path;
}

QString assFileSqlInfo::generateFileName(const QString &suffixes)
{
    QString name("%1%2%3");

    //首先格式化基本名称
    assTypePtr at_ = getAssType();
    if (at_)
        name = name.arg(at_->getAssType());
    else
        name = name.arg(QString());

    fileTypePtr ft_ = getFileType();
    if (ft_)
    {
        if (ft_->getFileType() == QString("rig"))
            name = name.arg(ft_->getFileType());
        else
            name = name.arg(QString());
    }
    else
    {
        name = name.arg(QString());
    }

    name = name.arg(suffixes);

    return name;
}

QString assFileSqlInfo::generateFileName(const QString &suffixes, const QString &prefix)
{
    QString name("%1_%2");
    name = name.arg(prefix);
    name = name.arg(generateFileName(suffixes));
    return name;
}

fileClassPtr assFileSqlInfo::getFileClass()
{
    if (p_ptrW_fileClass)
    {
        return p_ptrW_fileClass;
    }
    else
    {
        fileClassPtr p_ = fileClassPtr(new fileClass);
        p_->select(__file_class__);
        p_ptrW_fileClass = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setFileClass(const fileClassPtrW &fileclass_)
{
    if (!fileclass_)
        return;
    p_ptrW_fileClass = fileclass_;
    __file_class__ = fileclass_.lock()->getIdP();
}

fileTypePtr assFileSqlInfo::getFileType()
{
    if (p_ptrW_fileType)
    {
        return p_ptrW_fileType;
    }
    else
    {
        fileTypePtr p_ = fileTypePtr(new fileType);
        p_->select(__file_type__);
        p_ptrW_fileType = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setFileType(const fileTypePtrW &fileType_)
{
    if (!fileType_)
        return;
    p_ptrW_fileType = fileType_;
    __file_type__ = fileType_.lock()->getIdP();

    setAssType(fileType_.lock()->getAssType());
}

assTypePtr assFileSqlInfo::getAssType()
{
    if (p_ptrW_assType != nullptr)
    {
        return p_ptrW_assType;
    }
    else
    {
        assTypePtr p_ = assTypePtr(new assType);
        p_->select(__ass_class__);
        p_ptrW_assType = p_;
        return p_;
    }
    return nullptr;
}

void assFileSqlInfo::setAssType(const assTypePtrW &assType_)
{
    if (!assType_)
        return;
    p_ptrW_assType = assType_;
    __ass_class__ = assType_.lock()->getIdP();

    setFileClass(assType_.lock()->getFileClass());
}

CORE_DNAMESPACE_E
