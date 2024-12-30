from PySide2.QtCore import Qt, Signal
from PySide2.QtWidgets import QLabel, QHBoxLayout, QWidget


class SelectableItem(QWidget):
    on_clicked = Signal(object)
    def __init__(self, parent=None):
        super(SelectableItem, self).__init__(parent)
        self.layout = QHBoxLayout()
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.setObjectName('sidebar-item')
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.setMinimumHeight(30)
        self.label = ''
        self.setLayout(self.layout)
        self.name = QLabel()
        self.name.setObjectName("name")
        # self.name.setStyleSheet("text-indent: 2em;background-color: white")
        self.layout.addWidget(self.name)

        self.setCursor(Qt.PointingHandCursor)
        # self.setStyleSheet("padding-left: 10px;padding-right: 10px;background-color: rgba(255, 255, 255,0);")

        self.setStyleSheet("""
                    #sidebar-item {
                        background-color: rgba(255, 255, 255,0);
                    }

                    QLabel {
                        padding-left: 10px;
                        padding-right: 10px;
                    }

                    #sidebar-item:hover {
                        background-color: #5285a6;
                    }""")

    def set_selection(self, state):
        if state:
            self.setStyleSheet("""
            #sidebar-item {
                background-color: #5285a6;
            }
            #sidebar-item:hover {
                background-color: #5285a6;
                color: black; 
            }
            #name {
                color: black; 
            }

            QLabel {
                        padding-left: 10px;
                        padding-right: 10px;
                    }

            """)
        else:
            self.setStyleSheet("""
            #sidebar-item {
                background-color: rgba(255, 255, 255,0);
            }
            #sidebar-item:hover {
                background-color: #5285a6;
            }""")

    def mousePressEvent(self, event):
        super(SelectableItem, self).mousePressEvent(event)
        self.on_clicked.emit(self)
