from PySide2.QtCore import Signal
from PySide2.QtWidgets import QWidget
from doodle.cell.h_box_layout_widget import HBoxLayoutWidget


class DoodleScrollAreaItem(HBoxLayoutWidget):
    clicked = Signal()

    def __init__(self, parent=None):
        super(DoodleScrollAreaItem, self).__init__(parent)

    def mousePressEvent(self, event):
        super(DoodleScrollAreaItem, self).mousePressEvent(event)
        self.clicked.emit(self)
