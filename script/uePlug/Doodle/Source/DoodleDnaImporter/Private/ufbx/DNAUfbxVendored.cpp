// ufbx amalgamation, compiled once for this plugin.
//
// This translation unit intentionally includes no Unreal header: the ufbx implementation is a
// large C file that is compiled as C++ here, exactly like the engine does in
// Engine/Plugins/Interchange/Runtime/Source/Parsers/Fbx/Private/Ufbx/UfbxParser.cpp.
//
// The file is named *.inl so that UnrealBuildTool does not also compile it as its own
// translation unit (which would define every ufbx symbol twice and fail the link).

#include "ufbx.inl"
