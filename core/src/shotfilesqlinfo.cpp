#include "shotfilesqlinfo.h"

#include "sql_builder/sql.h"

#include "coreset.h"
#include "episodes.h"
#include "shot.h"
#include "fileclass.h"
#include "filetype.h"

#include <QVariant>
#include <QDebug>
#include <QSqlError>

#include <iostream>

CORE_NAMESPACE_S

shotFileSqlInfo::shotFileSqlInfo()
{
    __episodes__ = -1;
    __shot__ = -1;
    __file_class__ = -1;
    __file_type__ = -1;

    p_ptrw_eps = nullptr;
    p_ptrw_shot = nullptr;
    p_ptrw_fileClass = nullptr;
    p_ptrw_fileType = nullptr;
}

void shotFileSqlInfo::select(const qint64 &ID_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__episodes__", "__shot__", "__file_class__", "__file_type__");

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
        infoP = query->value(6).toByteArray();
        fileStateP = query->value(7).toString();

        if (!query->value(8).isNull())
            __episodes__ = query->value(8).toInt();

        if (!query->value(9).isNull())
            __shot__ = query->value(9).toInt();

        if (!query->value(10).isNull())
            __file_class__ = query->value(10).toInt();

        if (!query->value(11).isNull())
            __file_type__ = query->value(11).toInt();
    }
}

void shotFileSqlInfo::insert()
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

        if (__episodes__ > 0)
            ins_.insert("__episodes__", (unsigned char)__episodes__);
        if (__shot__ > 0)
            ins_.insert("__shot__", (unsigned char)__shot__);
        if (__file_class__ > 0)
            ins_.insert("__file_class__", (unsigned char)__file_class__);
        if (__file_type__ > 0)
            ins_.insert("__file_type__", (unsigned char)__file_type__);

        ins_.into(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());

        qDebug() << QString::fromStdString(ins_.str());
        if (!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());
        getInsertID(query);

        query->finish();
    }
}

void shotFileSqlInfo::updateSQL()
{
    sql::UpdateModel upd_;
    upd_.update(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    upd_.set("filestate", fileSuffixesP.toStdString());
    upd_.set("infor", infoP.toStdString());
    if ((__episodes__ >= 0) && (__episodes__ != p_ptrw_eps.lock()->getIdP()))
        upd_.set("__episodes__", __episodes__);

    if (__shot__ >= 0) {
      if (__shot__ != p_ptrw_shot.lock()->getIdP())
        upd_.set("__shot__", __shot__);
    }
    if ((__file_class__ >= 0) && (__file_class__ != p_ptrw_fileClass.lock()->getIdP()))
        upd_.set("__file_class__", __file_class__);
    if ((__file_type__ >= 0) && (__file_type__ != p_ptrw_fileType.lock()->getIdP()))
        upd_.set("__file_type__", __file_type__);

    upd_.where(sql::column("id") == idP);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(upd_.str())))
        throw std::runtime_error("not updata fileinfo");
    query->finish();
}

void shotFileSqlInfo::deleteSQL()
{
}

shotInfoPtrList shotFileSqlInfo::batchQuerySelect(sqlQuertPtr &query)
{
    shotInfoPtrList listShot;
    while (query->next())
    {
        shotInfoPtr ptr(new shotFileSqlInfo);

        ptr->idP = query->value(0).toInt();
        ptr->fileP = query->value(1).toString();
        ptr->fileSuffixesP = query->value(2).toString();
        ptr->userP = query->value(3).toString();
        ptr->versionP = query->value(4).toInt();
        ptr->filepathP = query->value(5).toString();
        ptr->infoP = query->value(6).toByteArray();
        ptr->fileStateP = query->value(7).toString();

        if (!query->value(8).isNull())
            ptr->__episodes__ = query->value(8).toInt();

        if (!query->value(9).isNull())
            ptr->__shot__ = query->value(9).toInt();

        if (!query->value(10).isNull())
            ptr->__file_class__ = query->value(10).toInt();

        if (!query->value(11).isNull())
            ptr->__file_type__ = query->value(11).toInt();

        listShot.append(ptr);
    }
    return listShot;
}

shotInfoPtrList shotFileSqlInfo::getAll(const episodesPtr &EP_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__episodes__", "__shot__", "__file_class__", "__file_type__");

    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__episodes__") == EP_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error("not exe query");

    shotInfoPtrList listInfo = batchQuerySelect(query);
    for (auto &x : listInfo)
    {
        x->setEpisdes(EP_);
    }
    return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr &sh_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__episodes__", "__shot__", "__file_class__", "__file_type__");

    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__shot__") == sh_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error("not exe query");

    shotInfoPtrList listInfo = batchQuerySelect(query);
    for (auto &x : listInfo)
    {
        x->setShot(sh_);
    }
    return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const fileClassPtr &fc_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__episodes__", "__shot__", "__file_class__", "__file_type__");

    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_class__") == fc_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error("not exe query");

    shotInfoPtrList listInfo = batchQuerySelect(query);
    for (auto &x : listInfo)
    {
        x->setFileClass(fc_);
    }
    return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const fileTypePtr &ft_)
{
    sql::SelectModel sel_;
    sel_.select("id", "file", "fileSuffixes", "user", "version",
                "_file_path_", "infor", "filestate",
                "__episodes__", "__shot__", "__file_class__", "__file_type__");

    sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("__file_type__") == ft_->getIdP());

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str())))
        throw std::runtime_error("not exe query");

    shotInfoPtrList listInfo = batchQuerySelect(query);
    for (auto &x : listInfo)
    {
        x->setFileType(ft_);
    }
    return listInfo;
}

QString shotFileSqlInfo::generatePath(const QString &programFolder)
{
    QString str("%1/%2/%3/%4/%5/%6");
    coreSet &set = coreSet::getCoreSet();
    //第一次格式化添加根路径
    str = str.arg(set.getShotRoot().absolutePath());
    //第二次格式化添加集数字符串
    episodesPtr ep_ = getEpisdes();
    if (ep_ != nullptr)
        str = str.arg(ep_->getEpisdes_str());
    else
        str = str.arg(QString());

    //第三次格式化添加镜头字符串
    shotPtr sh_ = getShot();
    if (sh_ != nullptr)
        str = str.arg(sh_->getShotAndAb_str());
    else
        str = str.arg(QString());

    //第四次格式化添加程序文件夹
    str = str.arg(programFolder);

    //第五次格式化添加部门文件夹
    fileClassPtr fc_ = getFileclass();
    if (fc_ != nullptr)
        str = str.arg(fc_->getFileclass_str());
    else
        str = str.arg(QString());

    //第六次格式化添加类别文件夹
    fileTypePtr ft_ = getFileType();
    if (ft_ != nullptr)
        str = str.arg(ft_->getFileType());
    else
        str = str.arg(QString());

    return formatPath(str);
}

QString shotFileSqlInfo::generatePath(const QString &programFolder, const QString &suffixes)
{
    QString str("%1/%2");
    str = str.arg(generatePath(programFolder));

    str = str.arg(generateFileName(suffixes));

    return str;
}

QString shotFileSqlInfo::generatePath(const QString &programFolder, const QString &suffixes, const QString &prefix)
{
    QString str("%1/%2");
    str = str.arg(generatePath(programFolder));

    str = str.arg(generateFileName(suffixes, prefix));

    return str;
}

QString shotFileSqlInfo::generateFileName(const QString &suffixes)
{

    QString name("shot_%1_%2_%3_%4_v%5_%6.%7");
    //第一次 格式化添加 集数
    episodesPtr ep_ = getEpisdes();
    if (ep_ != nullptr)
        name = name.arg(ep_->getEpisdes_str());
    else
        name = name.arg(QString());

    //第二次格式化添加 镜头号
    shotPtr sh_ = getShot();
    if (sh_ != nullptr)
        name = name.arg(sh_->getShotAndAb_str());
    else
        name = name.arg(QString());

    //第三次格式化添加 fileclass
    fileClassPtr fc_ = getFileclass();
    if (fc_ != nullptr)
        name = name.arg(fc_->getFileclass_str());
    else
        name = name.arg(QString());

    //第四次格式化添加 fileType
    fileTypePtr ft_ = getFileType();
    if (ft_ != nullptr)
        name = name.arg(ft_->getFileType());
    else
        name = name.arg(QString());

    name = name.arg(versionP, 4, 10, QLatin1Char('0'));
    name = name.arg(coreSet::getCoreSet().getUser_en());
    name = name.arg(suffixes);

    return name;
}

QString shotFileSqlInfo::generateFileName(const QString &suffixes, const QString &prefix)
{
    QString name("%1_%2");
    name = name.arg(prefix);
    name = name.arg(generateFileName(suffixes));
    return name;
}

episodesPtr shotFileSqlInfo::getEpisdes()
{
    if (p_ptrw_eps)
    {
        return p_ptrw_eps;
    }
    else if (__episodes__ >= 0)
    {
        episodesPtr p_ = episodesPtr(new episodes);
        p_->select(__episodes__);
        this->setEpisdes(p_);
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setEpisdes(const episodesPtrW &eps_)
{
    if (!eps_)
        return;

    p_ptrw_eps = eps_;
    __episodes__ = eps_.lock()->getIdP();
}

shotPtr shotFileSqlInfo::getShot()
{
    if (p_ptrw_shot != nullptr)
    {
        return p_ptrw_shot;
    }
    else if (__shot__ >= 0)
    {
        shotPtr p_ = shotPtr(new shot);
        p_->select(__shot__);
        p_ptrw_shot = p_.toWeakRef();
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setShot(const shotPtrW &shot_)
{
    if (!shot_)
        return;
    p_ptrw_shot = shot_;
    __shot__ = shot_.lock()->getIdP();

    setEpisdes(shot_.lock()->getEpisodes());
}

fileClassPtr shotFileSqlInfo::getFileclass()
{
    if (p_ptrw_fileClass)
        return p_ptrw_fileClass;
    else if (__file_class__ >= 0)
    {
        fileClassPtr p_ = fileClassPtr(new fileClass);
        p_->select(__file_class__);
        p_ptrw_fileClass = p_;
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setFileClass(const fileClassPtrW &value)
{
    if (!value)
        return;
    __file_class__ = value.lock()->getIdP();
    p_ptrw_fileClass = value;

    setShot(value.lock()->getShot());
}

fileTypePtr shotFileSqlInfo::getFileType()
{
    if (p_ptrw_fileType)
        return p_ptrw_fileType;
    else if (__file_type__ >= 0)
    {
        fileTypePtr p_ = fileTypePtr(new fileType);
        p_->select(__file_type__);
        p_ptrw_fileType = p_;
        return p_;
    }
    return nullptr;
}

void shotFileSqlInfo::setFileType(const fileTypePtrW &fileType_)
{
    if (!fileType_)
        return;
    __file_type__ = fileType_.lock()->getIdP();
    p_ptrw_fileType = fileType_;

    setFileClass(fileType_.lock()->getFileClass());
}

CORE_NAMESPACE_E
