from PySide2.QtCore import Qt
from PySide2.QtWidgets import QWidget, QHBoxLayout


class HBoxLayoutWidget(QWidget):
    def __init__(self, parent=None,is_style=False):
        super(HBoxLayoutWidget, self).__init__(parent=parent)
        self.main_layout = QHBoxLayout()
        self.main_widget = QWidget()
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.layout = QHBoxLayout()
        self.main_widget.setLayout(self.layout)
        self.setLayout(self.main_layout)
        self.main_layout.addWidget(self.main_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self._styles = None


    @property
    def styles(self):
        return self._styles


    @styles.setter
    def styles(self, value):
        self._styles = value
        self.setStyleSheet(self._styles)

class HBoxLayoutLeftWidget(HBoxLayoutWidget):
    def __init__(self, parent=None):
        super(HBoxLayoutLeftWidget, self).__init__(parent=parent)
        self.layout.setAlignment(Qt.AlignLeft)
        self.main_layout.setAlignment(Qt.AlignLeft)


class HBoxLayoutRightWidget(HBoxLayoutWidget):
    def __init__(self, parent=None):
        super(HBoxLayoutRightWidget, self).__init__(parent=parent)
        self.layout.setAlignment(Qt.AlignRight)
        self.layout.setAlignment(Qt.AlignRight)


class HBoxLayoutCenterWidget(HBoxLayoutWidget):
    def __init__(self, parent=None):
        super(HBoxLayoutCenterWidget, self).__init__(parent)
        self.layout.setAlignment(Qt.AlignCenter)
        self.main_layout.setAlignment(Qt.AlignCenter)
