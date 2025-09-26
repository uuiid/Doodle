from PySide2.QtCore import Qt, QTimer
from PySide2.QtWidgets import QWidget, QHBoxLayout, QLabel, QSlider, QDoubleSpinBox
from doodle.doodle_maya import State


class NoWheelSlider(QSlider):

    def wheelEvent(self, event):
        event.ignore()


class ModifyFacePartsCell(QWidget):
    def __init__(self):
        super(ModifyFacePartsCell, self).__init__()
        self.layout = QHBoxLayout()
        self.max_value = 5
        self.min_value = -5
        self.controllers = []
        self.direction = 'X'
        self.name = QLabel()
        self.name.setFixedWidth(70)
        self.value = QDoubleSpinBox()
        self.value.setRange(self.min_value, self.max_value)
        self.value.setDecimals(5)
        self.old_value = 0
        self.value.setSingleStep(0.00001)
        self.slider_value = NoWheelSlider()
        self.slider_value.setRange(self.min_value * 100000, self.max_value * 100000)
        self.slider_value.setValue(0)
        self.slider_value.setOrientation(Qt.Horizontal)
        self.layout.addWidget(self.name)
        self.layout.addWidget(self.slider_value)
        self.layout.addWidget(self.value)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.layout)
        self.slider_value.sliderMoved.connect(self.on_slider_value_changed)
        self.slider_value.sliderReleased.connect(self.on_slider_released)
        self.slider_value.sliderPressed.connect(self.on_slider_pressed)
        self.value.valueChanged.connect(self.on_value_changed)
        # self.timer = QTimer()
        # self.timer.timeout.connect(self.update)

    # def update(self):
    #     if self.controllers:
    #         controller = self.controllers[-1]
    #         value = State.get_controller_value(controller + '.translate' + self.direction)
    #         print(value)
    #         if self.direction == 'Y':
    #             if '_L_' in controller:
    #                 value = value * -1
    #         # self.value.setValue(float(value))
    #         self.slider_value.setValue(value * 100000)

    def on_slider_pressed(self):
        print(self.slider_value.value() / 100000.0)
        if abs(self.slider_value.value() / 100000.0) < 0.05:
            self.slider_value.setValue(0)

    def on_slider_released(self):
        self.value.setValue(float(self.slider_value.value()) / 100000.0)

    def on_slider_value_changed(self, value):
        self.value.setValue(float(value) / 100000.0)

    def on_value_changed(self, value):
        self.slider_value.setValue(value * 100000)
        for controller in self.controllers:
            temp_value = value
            if self.direction == 'Y':
                if '_L_' in controller:
                    temp_value = value * -1
            if self.direction == 'Y' and '_M_' in controller:
                continue
            # print((controller + '.translate' + self.direction).decode('utf-8'))
            State.set_controller_value(controller + '.translate' + self.direction, temp_value)
            # cmds.setAttr(controller + '.translate' + self.direction, value)
