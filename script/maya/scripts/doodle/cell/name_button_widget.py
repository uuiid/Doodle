from PySide2.QtCore import Signal
from PySide2.QtWidgets import QPushButton, QLabel

from doodle.cell.h_box_layout_widget import HBoxLayoutWidget


class NameButtonWidget(HBoxLayoutWidget):
    on_remove = Signal(object)
    on_clicked = Signal(object)
    def __init__(self, parent=None):
        super(NameButtonWidget, self).__init__(parent)
        self.name = QLabel(self)
        self.label = None
        self.layout.setContentsMargins(0, 0, 10, 0)
        self.button = QPushButton(self)
        self.button.setMaximumWidth(20)
        self.button.setMaximumHeight(20)
        self.layout.addWidget(self.name)
        self.layout.addWidget(self.button)
        self.setMinimumHeight(30)
        self.button.clicked.connect(self.button_clicked)

    def button_clicked(self,*args):
        self.on_remove.emit(self.label)
        self.deleteLater()

    def mousePressEvent(self, event):
        super(NameButtonWidget, self).mousePressEvent(event)
        self.on_clicked.emit(self.label)