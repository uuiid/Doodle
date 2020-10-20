#include "fileArchiveOneFile.h"
//设置导入
#include "coreset.h"
//错误类型导入
#include "doException/doException.h"

#include "filesqlinfo.h"

//ftp模块导入
#include "src/ftpsession.h"

#include <QFileInfo>
#include <QVector>
CORE_NAMESPACE_S
void fileArchiveOneFile::copyToCache() const
{
    assert(p_soureFile.size() == 1);
    if (p_cacheFilePath.exists()) //进行检查存在性,  存在即删除
    {
        QFile f;
        f.setFileName(p_cacheFilePath.filePath());
        f.remove();
    }
    //复制文件  如果无法复制就抛出异常
    QFile soureFile_;
    soureFile_.setFileName(p_soureFile[0].filePath());
    if (!soureFile_.copy(p_cacheFilePath.filePath()))
        throw doodle_CopyErr(p_soureFile[0].filePath().toStdString());
}

bool fileArchiveOneFile::isInCache()
{
    assert(p_soureFile.size() == 1);
    coreSet &set = coreSet::getCoreSet();
    if (!p_soureFile[0].exists())
        throw doodle_notFile(p_soureFile[0].filePath().toStdString());

    //获得缓存路径
    QDir dir((set.getCacheRoot().path() + p_Path));
    if (!dir.exists()) //不存在缓存目录
    {
        dir.mkdir(dir.absolutePath()); //制作目录
    }
    else //存在目录
    {
        p_cacheFilePath.setFile(dir, p_soureFile[0].fileName());
        if (p_cacheFilePath.exists()) //进行检查存在性,
        {
            return true;
        }
    }
    return false;
}

void fileArchiveOneFile::insertArchive() 
{
    
}

void fileArchiveOneFile::_updata()
{
    coreSet &set = coreSet::getCoreSet();
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(set.getIpFtp(), 21, set.getProjectname() + set.getUser_en(), set.getUser_en());
    if (!session->upload(p_cacheFilePath.absoluteFilePath(), p_Path))
        throw doodle_upload_error(p_cacheFilePath.absoluteFilePath().toStdString());
}
CORE_NAMESPACE_E