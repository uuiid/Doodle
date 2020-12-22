#pragma once

#include <core_global.h>
#include <src/fileArchive/mayaArchive.h>

CORE_NAMESPACE_S

class CORE_API assMayaArchive : public mayaArchive {
 public:
  assMayaArchive(fileSqlInfoPtr f_ptr);
};

CORE_NAMESPACE_E