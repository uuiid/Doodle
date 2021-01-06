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
#include <core_global.h>
#include <src/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S
class CORE_API ueSynArchive : public fileArchive {
 public:
  ueSynArchive();
  dpath syn(const shotPtr& shot_);
  bool update() override;
  bool makeDir(const episodesPtr& episodes_ptr);

 protected:
  void insertDB() override;
  void _generateFilePath() override;

 private:
  freeSynWrapPtr p_syn;
  synPathListPtr synpart;
};

DOODLE_NAMESPACE_E