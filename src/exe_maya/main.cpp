//
// Created by td_main on 2023/4/25.
//
#include <doodle_lib/facet/create_move_facet.h>
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/rpc_server_facet.h>

#include "maya/MApiNamespace.h"
#include <maya/MGlobal.h>
#include <maya/MLibrary.h>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main() try {
  MLibrary::initialize(true, "maya_doodle");
  MLibrary::cleanup(0, false);
} catch (...) {
  return 1;
}
