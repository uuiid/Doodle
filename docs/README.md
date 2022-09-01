# doodle 文档

## 导航

### 文件下载

[打开列表](/download_files/index.html ':ignore')

### 代码文档

[doc](/DOXYGEN_DOC/index.html ':ignore')  
[更新日志](/DOXYGEN_DOC/update.html ':ignore')

[开发进度](schedule.md)  
[开发计划](development_plan.md)


<details>  
<summary>本地 doc 文档 apache 配置 </summary>  

``` xml
Define DOCROOT "C:/Users/TD/Source/Doodle"
DocumentRoot "${DOCROOT}/docs"
<Directory "${DOCROOT}/docs">
    Options Indexes MultiViews FollowSymlinks
    AllowOverride None
</Directory>

Alias "/download_files" "${DOCROOT}/build/html/file"
<Directory "${DOCROOT}/build/html/file">
    Options Indexes FollowSymLinks MultiViews
    AllowOverride None
    Require all granted
</Directory>

Alias "/DOXYGEN_DOC" "${DOCROOT}/build/html"
<Directory "${DOCROOT}/build/html">
    Options Indexes FollowSymLinks MultiViews
    AllowOverride None
    Require all granted
</Directory>

```

<summary>c++ 示例 </summary>
</details>
