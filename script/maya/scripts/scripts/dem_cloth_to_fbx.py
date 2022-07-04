from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *


class dem_cloth_to_fbx(QWidget):

    def __init__(self):
        super(dem_cloth_to_fbx, self).__init__()

        self.nbone = 30

        self.setObjectName("dem_cloth_to_fbx")
        self.setWindowTitle("转换骨骼物体")
        self.setGeometry(50, 50, 250, 150)
        self.initUI()

    def initUI(self):
        self.layout = QHBoxLayout(self)
        self.lable = QLabel().setText("骨骼数")
        self.layout.addWidget(self.lable)
        self.nbone_w = QSpinBox(self)
        self.nbone_w.setValue(self.nbone)
        self.nbone_w.valueChanged.connect(lambda x: self.set_bones(x))
        self.layout.addWidget(self.nbone_w)

        self.setLayout(self.layout)

    def set_bones(self, n):
        self.nbone = n
