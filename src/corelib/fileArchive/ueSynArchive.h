/*
 * @Author: your name
 * @Date: 2020-11-11 16:20:03
 * @LastEditTime: 2020-11-26 17:56:06
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\ueSynArchive.h
 */
//
// Created by teXiao on 2020/11/11.
//
#pragma once
#include <corelib/core_global.h>
#include <corelib/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S



class CORE_API ueSynArchive : public fileArchive {
 public:
  ueSynArchive();
  dpath syn(const episodesPtr& episodes_ptr, const shotPtr& shot_ptr);
  bool update() override;
  bool makeDir(const episodesPtr& episodes_ptr);

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

 private:
  synPathListPtr synpart;
};

DOODLE_NAMESPACE_E