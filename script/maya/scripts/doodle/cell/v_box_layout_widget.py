from PySide2.QtCore import Qt
from PySide2.QtWidgets import QWidget, QVBoxLayout


class VBoxLayoutWidget(QWidget):
    def __init__(self, parent=None):
        super(VBoxLayoutWidget, self).__init__(parent)
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.main_layout = QVBoxLayout()
        self.main_widget = QWidget()
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.layout = QVBoxLayout()
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

class VBoxLayoutTopWidget(VBoxLayoutWidget):
    def __init__(self, parent=None):
        super(VBoxLayoutTopWidget, self).__init__(parent)
        self.layout.setAlignment(Qt.AlignTop)


class VBoxLayoutBottomWidget(VBoxLayoutWidget):
    def __init__(self, parent=None):
        super(VBoxLayoutBottomWidget, self).__init__(parent)
        self.layout.setAlignment(Qt.AlignBottom)


