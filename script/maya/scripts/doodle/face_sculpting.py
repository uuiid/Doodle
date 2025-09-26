# -*- coding: utf-8 -*-
from PySide2.QtWidgets import QWidget, QVBoxLayout
import sys
from doodle.cell.modify_face_parts_cell import ModifyFacePartsCell
from PySide2.QtWidgets import QApplication
from doodle.widgets.doodle_scroll_area import DoodleScrollArea
from PySide2.QtCore import QSize, Qt


class FaceSculptingWidget(QWidget):
    def __init__(self, parent=None):
        super(FaceSculptingWidget, self).__init__(parent=parent)
        self.layout = QVBoxLayout()
        self.setWindowFlags(self.windowFlags() | Qt.Window)
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.scroll_area = DoodleScrollArea()
        self.layout.addWidget(self.scroll_area)
        self.setLayout(self.layout)
        self.init_widget()
        self.resize(QSize(800, 600))

    def init_widget(self):
        face_sculpting_parts = [
            {
                'name': '眼部',
                'value': 0,
                'controllers': ['jnt_MeiTou_R_001Con', 'jnt_MeiMao_R_001Con', 'jnt_MeiWei_R_001Con',
                                'jnt_MeiTou_L_001Con', 'jnt_MeiMao_L_001Con', 'jnt_MeiWei_L_001Con',
                                'jnt_YanJing_R_001Con', 'jnt_YanPiShang_L_001Con', 'jnt_YanPiShang_R_001Con',
                                'jnt_YanJing_L_001Con',
                                'jnt_YanPiShang_R_002Con',
                                'jnt_YanPiShang_L_002Con', 'jnt_YanPiXia_R_002Con',
                                'jnt_YanPiXia_L_002Con',
                                'jnt_YanPiXia_R_001Con', 'jnt_YanPiXia_L_001Con', 'jnt_YanWo_R_001Con',
                                'jnt_YanWo_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '左右': 'Y'
                    }
                ]

            },
            {
                'name': '眉毛',
                'value': 0,
                'controllers': ['jnt_MeiTou_R_001Con', 'jnt_MeiMao_R_001Con', 'jnt_MeiWei_R_001Con',
                                'jnt_MeiTou_L_001Con', 'jnt_MeiMao_L_001Con', 'jnt_MeiWei_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '左右': 'Y'
                    }
                ]

            },
            {
                'name': '眉心',
                'value': 0,
                'controllers': ['jnt_MeiJian_M_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    }
                ]
            },
            {
                'name': '鼻根',
                'value': 0,
                'controllers': ['jnt_BiLiang_M_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    }
                ]
            },
            {
                'name': '鼻梁',
                'value': 0,
                'controllers': ['jnt_BiFeng_M_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    }
                ]
            },
            {
                'name': '鼻梁翼',
                'value': 0,
                'controllers': ['jnt_BiLiangCe_R_001Con', 'jnt_BiLiangCe_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y'
                    }
                ]
            },
            {
                'name': '鼻尖',
                'value': 0,
                'controllers': ['jnt_BiJian_M_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    }
                ]
            },
            {
                'name': '鼻头翼',
                'value': 0,
                'controllers': ['jnt_BiYi_L_001Con', 'jnt_BiYi_R_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y'
                    }
                ]
            },
            {
                'name': '嘴巴',
                'value': 0,
                'controllers': ['jnt_ShangChun_M_001Con', 'jnt_XiaChun_M_001Con', 'jnt_ShangZuiChun_L_001Con',
                                'jnt_XiaZuiChun_L_001Con', 'jnt_ZuiJiao_L_001Con', 'jnt_ShangZuiChun_R_001Con',
                                'jnt_XiaZuiChun_R_001Con', 'jnt_ZuiJiao_R_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y',
                    }
                ]
            },
            {
                'name': '下巴',
                'value': 0,
                'controllers': ['jnt_XiaBaCe_R_001Con', 'jnt_XiaBa_M_001Con', 'jnt_XiaBaCe_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y',
                    }
                ]
            },
            {
                'name': '颧骨',
                'value': 0,
                'controllers': ['jnt_QuanGu_R_001Con', 'jnt_QuanGu_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y',
                    }
                ]
            },
            {
                'name': '脸',
                'value': 0,
                'controllers': ['jnt_RianJia_R_001Con', 'jnt_LianJia_L_001Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y',
                    }
                ]
            }
            ,{
                'name': '下颚',
                'value': 0,
                'controllers': ['jnt_XiaEGu_R_002Con', 'jnt_XiaEGu_L_002Con'],
                'direction': [
                    {
                        '上下': 'X',
                    },
                    {
                        '前后': 'Z'
                    },
                    {
                        '宽度': 'Y',
                    }
                ]
            }

        ]
        for i in face_sculpting_parts:
            {

            }.keys()
            for j in i['direction']:
                cell = ModifyFacePartsCell()
                cell.name.setText(i['name'] + j.keys()[0])
                cell.controllers = i['controllers']
                cell.direction = j.values()[0]
                self.scroll_area.add_widget(cell)


if __name__ == '__main__':
    APP = QApplication(sys.argv)
    gui = FaceSculptingWidget()
    gui.show()
    APP.exec_()
