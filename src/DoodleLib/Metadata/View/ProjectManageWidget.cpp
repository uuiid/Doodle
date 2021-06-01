//
// Created by TD on 2021/6/1.
//

#include <DoodleLib/Metadata/View/ProjectManageWidget.h>

#include <wx/dataview.h>
namespace doodle {
ProjectManageWidget::ProjectManageWidget() {
  new wxDataViewListCtrl{};
}

}  // namespace doodle