#include "time_sequencer_widget.h"

#include <implot.h>
#include <implot_internal.h>
namespace doodle::gui {
class time_sequencer_widget::impl {
 public:
  ~impl(){ImPlot::DestroyContext();}
  impl(){ImPlot::CreateContext();}
};

time_sequencer_widget::time_sequencer_widget()
    : p_i(std::make_unique<impl>()) {
}

time_sequencer_widget::~time_sequencer_widget() = default;

void time_sequencer_widget::init() {
}
void time_sequencer_widget::succeeded() {
}
void time_sequencer_widget::failed() {
}
void time_sequencer_widget::aborted() {
}

void time_sequencer_widget::update(
    const chrono::duration<chrono::system_clock::rep,
                           chrono::system_clock::period>&,
    void* in_data) {

}
}  // namespace doodle::gui
