# coding:utf-8
import os
import sys
import traceback
import json

# 加载ct_plu库
sys.path.insert(0, "c:/CgTeamWork_v7/bin/base/ct_plu/")
sys.path.append("c:/CgTeamWork_v7/bin/base")
from cgtw2 import *
import ct_plu
from ct_plu.qt import *  # 导入QT的库

class widget_ui(QDialog):
    m_res = False

    def __init__(self, parent=None):
        super(widget_ui, self).__init__(parent)
        self.setWindowFlags(Qt.FramelessWindowHint)  # 无边框

    def assert_map(self):
        file = os.path.basename(self.file_path)
        isFit = False
        if file == self.zymc + "_Low.ma":
            isFit = True
        if file.startswith(self.zymc+"_") and file.endswith("_Low.ma"):
            isFit = True
        if not isFit:
            QMessageBox.critical(self, "错误", "ma文件命名必须为:%s(_版本)_Low" % self.zymc)
            return False
        self.m_res = True

    def assert_mesh(self):
        file = os.path.basename(self.file_path)
        isFit = False
        if file == self.zymc + ".ma":
            isFit = True
        if file.startswith(self.zymc+"_") and file.endswith(".ma"):
            isFit = True
        if not isFit:
            QMessageBox.critical(self, "错误", "ma文件命名必须为:%s(_版本)" % self.zymc)
            return False
        self.m_res = True

    def assert_mod(self):
        if len(self.bh)<=0:
            QMessageBox.critical(self, "错误", "编号不能为空")
        zymc_file = "Ch"+ self.bh +".ma"
        if not os.path.basename(self.file_path) == zymc_file:
            QMessageBox.critical(self, "错误", "ma文件命名必须为%s" % zymc_file)
            return False
        self.m_res = True

    def assert_rig(self):
        if len(self.zclx)<=0:
            QMessageBox.critical(self, "错误", "资产类型不能为空")
            return False
        if len(self.bh)<=0:
            QMessageBox.critical(self, "错误", "编号不能为空")
            return False
        f_name = os.path.basename(self.file_path)
        f_name2,ext = os.path.splitext(f_name)
        _zzz = f_name2.split("_")[-1]
        if self.zclx == "Chars":
            if len(f_name2.split("_"))<3:
                QMessageBox.critical(self, "命名错误", "格式为：Ch(编号)_rig_(制作人).ma")
                return False
            zymc_file = "Ch"+ self.bh +"_rig_"+ _zzz +".ma"
            if not os.path.basename(self.file_path) == zymc_file:
                QMessageBox.critical(self, "命名错误", "ma文件命名必须为%s" % zymc_file)
                return False
        if self.zclx == "Props":
            if len(f_name2.split("_"))<3:
                QMessageBox.critical(self, "命名错误", "格式为：资源名称(_版本)_rig_(制作人).ma")
                return False
            isFit = False
            if f_name == self.zymc +"_rig_"+ _zzz +".ma":
                isFit = True
            if f_name.startswith(self.zymc+"_") and f_name.endswith("_rig_"+ _zzz +".ma"):
                isFit = True
            if not isFit:
                QMessageBox.critical(self, "命名错误", "格式为：资源名称(_版本)_rig_(制作人).ma")
                return False
        if self.zclx == "Scene":
            isFit = False
            if f_name == self.zymc +"_Low" +".ma":
                isFit = True
            if f_name.startswith(self.zymc+"_") and f_name.endswith("_Low"+".ma"):
                isFit = True
            if not isFit:
                QMessageBox.critical(self, "命名错误", "格式为：资源名称(_版本)_Low.ma")
                return False
        self.m_res = True

    def assert_fbx(self):
        f_name = os.path.basename(self.file_path)
        f_name2,ext = os.path.splitext(f_name)
        a = ord(f_name2[-1])
        if not (a in range(65,91) or a in range(48,58)):
            QMessageBox.critical(self, "文件名错误", "(镜头号)结尾必须为大写字母或数字")
            return False
        _js = self.js[-3:]
        _jth = self.jth[-3:]
        if a in range(65,91): #up
            zymc_file = self.p_code+"_EP"+_js + "_SC"+_jth + chr(a) + ".ma"
        else:
            zymc_file = self.p_code+"_EP"+_js + "_SC"+_jth + ".ma"
        if not os.path.basename(self.file_path) == zymc_file:
            QMessageBox.critical(self, "文件名错误", "ma文件命名必须为%s" % zymc_file)
            return False
        self.m_res = True
    
    def assert_cache(self):
        f_name = os.path.basename(self.file_path)
        f_name2,ext = os.path.splitext(f_name)
        a = ord(f_name2[-1])
        if not (a in range(65,91) or a in range(48,58)):
            QMessageBox.critical(self, "文件名错误", "(镜头号)结尾必须为大写字母或数字")
            return False
        _js = self.js[-3:]
        _jth = self.jth[-3:]
        if a in range(65,91): #up
            zymc_file = self.p_code+"_EP"+_js + "_SC"+_jth + chr(a) + ".ma"
        else:
            zymc_file = self.p_code+"_EP"+_js + "_SC"+_jth + ".ma"
        if not os.path.basename(self.file_path) == zymc_file:
            QMessageBox.critical(self, "文件名错误", "ma文件命名必须为%s" % zymc_file)
            return False
        self.m_res = True

    def run(self, a_dict_data):
        t_argv = ct_plu.argv(a_dict_data)
        self.setMinimumSize(800, 400)
        t_version_id = t_argv.get_version_id()  # 获取版本ID
        t_database = t_argv.get_sys_database()  # 获取数据库
        t_id_list = t_argv.get_sys_id()  # 获取界面选择ID列表
        t_module = t_argv.get_sys_module()  # 获取当前模块
        t_module_type = t_argv.get_sys_module_type()  # 获取当前模块类型
        t_file_list = t_argv.get_sys_file()  # 获取拖入的源文件列表
        t_des_file_list = t_argv.get_sys_des_file()  # 获取拖入后的目标文件列表
        t_folder_path = t_argv.get_sys_folder()  # 获取文件框所在的目录路径
        t_filebox_id = t_argv.get_sys_filebox_id()  # 获取文件框ID
        k_hello = t_argv.get_argv_key("hello")  # 获取当前插件配置的参数
        #-----------------
        self.file_path = ""
        for t_file in t_file_list:
            if os.path.splitext(t_file)[-1]== ".ma":
                self.file_path = t_file
        if len(self.file_path)<=0:
            QMessageBox.critical(self, "错误", "必须是.ma文件")
            return False
        #----------------
        t_tw = tw()
        self.zymc = ''
        self.bh=''
        self.ue_major_version =''
        self.zzz = ''
        self.p_code = ''
        self.js = ''
        self.jth = ''
        self.zclx = ''
        module= t_module
        res = t_tw.task.fields(t_database, module)
        field_sign_list = ["eps.entity","eps.project_code","task.version_data",
                           "pipeline.abridge","task.artist"] # 资源名称
        if t_module == "shot":
            field_sign_list.append("shot.entity")
        if t_module == "asset":
            field_sign_list.append("asset.entity")
            field_sign_list.append("asset_type.entity")
            field_sign_list.append("asset.number")
            field_sign_list.append("asset.ue_major_version")
        for field_sign in field_sign_list:
            if not field_sign in res:
                QMessageBox.critical(self, "错误", "缺少%s字段" % field_sign)
                return
        self.start_num = 0
        res = []
        _limit = 5000
        s_res = t_tw.task.get_filter(t_database, module, field_sign_list, [], limit=str(_limit), order_sign_list=[], start_num=str(self.start_num))
        while len(s_res)>0:
            res.extend(s_res)
            if len(s_res) >= _limit:
                self.start_num += len(s_res)
                s_res = t_tw.task.get_filter(t_database, module, field_sign_list, [], limit=str(_limit), order_sign_list=[], start_num=str(self.start_num))
            else:
                s_res = []
        for r in res:
            if r['id'] == t_id_list[0]:
                self.name = r['pipeline.abridge']
                self.zzz = r['task.artist']
                self.p_code = r['eps.project_code']
                self.js = r['eps.entity']
                if t_module == "shot":
                    self.jth = r['shot.entity']
                if t_module == "asset":
                    self.zclx = r['asset_type.entity']
                    self.zymc = r['asset.entity']
                    self.bh = r['asset.number']
                    self.ue_major_version = r['asset.ue_major_version']
        #-----------------------------
        if self.name == "Map":
            if len(self.zclx)<=0:
                QMessageBox.critical(self, "错误", "资产类型不存在")
                return False
            if self.zclx == "Props":
                self.assert_mesh()
            else:
                self.assert_map()
        if self.name == "Mod":
            self.assert_mod()
        if self.name == "Rig":
            self.assert_rig()
        if self.name == "Fbx":
            self.assert_fbx()
        if self.name == "Cache":
            self.assert_cache()
        return self.m_res

    # ct_base类名是固定的
class ct_base(ct_plu.extend):
    def __init__(self):
        ct_plu.extend.__init__(self)  # 继承

    # 重写run,外部调用
    def run(self, a_dict_data):
        try:
            t_res = widget_ui().run(a_dict_data)
            # 最后要返回
            if t_res == False:
                return self.ct_false("False")

        except:
            # print traceback.format_exc()
            return self.ct_false(traceback.format_exc())


if __name__ == "__main__":
    app = QApplication(sys.argv)
    # 调试数据,前提需要在拖入进程中右键菜单。发送到调试
    t_debug_argv_dict = ct_plu.argv().get_debug_argv_dict()
    print(ct_base().run(t_debug_argv_dict))
    app.exec_()