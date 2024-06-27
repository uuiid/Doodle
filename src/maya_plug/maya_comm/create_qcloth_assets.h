//
// Created by TD on 2022/7/20.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MPxContextCommand.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxToolCommand.h>

namespace doodle::maya_plug {
namespace create_qcloth_assets_ns {
constexpr char name[]     = "doodle_create_qcloth_assets";
constexpr char name_ctx[] = "doodle_create_qcloth_assets_context";
MSyntax syntax();
}  // namespace create_qcloth_assets_ns
class create_qcloth_assets
    : public TemplateAction<create_qcloth_assets, create_qcloth_assets_ns::name, create_qcloth_assets_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;

  void parse_arg(const MArgList& in_arg);
  static std::vector<MObject> get_all_node();

  void delete_node();
  void reset_properties();

  void filter_create_node(const std::vector<MObject>& in_obj);

 public:
  create_qcloth_assets();
  ~create_qcloth_assets() override;
  [[maybe_unused]] MStatus doIt(const MArgList& in_arg) override;

  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
  [[maybe_unused]] [[nodiscard]] bool isUndoable() const override;
};

class create_qcloth_assets_comm : public MPxToolCommand {
 public:
  create_qcloth_assets_comm() = default;
  static void* creator();
  MStatus doIt(const MArgList& in_arg) override;
  MStatus redoIt() override;
  MStatus undoIt() override;
  [[nodiscard]] bool isUndoable() const override;
  [[nodiscard]] bool hasSyntax() const override;
};

class create_qcloth_assets_context : public MPxSelectionContext {
 public:
  create_qcloth_assets_context();
  ~create_qcloth_assets_context() override;
  void toolOnSetup(MEvent& in_event) override;
  void toolOffCleanup() override;
  void getClassName(MString& in_name) const override;

  void deleteAction() override;
  void completeAction() override;
  void abortAction() override;
};

class create_qcloth_assets_context_comm : public MPxContextCommand {
 public:
  create_qcloth_assets_context_comm() = default;
  static void* creator();
  MPxContext* makeObj() override;

  template <class FNPLUG>
  static MStatus registerCommand(FNPLUG& obj) {
    return obj.registerContextCommand(
        create_qcloth_assets_ns::name_ctx, creator, create_qcloth_assets_ns::name, &create_qcloth_assets_comm::creator
    );
  }
  template <class FNPLUG>
  static MStatus deregisterCommand(FNPLUG& obj) {
    return obj.deregisterContextCommand(create_qcloth_assets_ns::name_ctx, create_qcloth_assets_ns::name);
  }
};

}  // namespace doodle::maya_plug
