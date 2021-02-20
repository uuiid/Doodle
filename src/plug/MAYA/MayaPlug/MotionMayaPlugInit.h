#pragma once

#include <maya/MGlobal.h>
#include <maya/MApiNamespace.h>

#include <maya/MPxCommand.h>

class doodletest : public MPxCommand {
 public:
  doodletest(){};
  virtual ~doodletest(){};

  static void* create() {
    return new doodletest();
  };

  virtual MStatus doIt(const MArgList& list) override {
    MGlobal::displayInfo("how word");
    return MStatus::kFailure;
  };
};