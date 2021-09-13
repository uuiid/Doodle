# -*- coding: utf-8 -*-

import maya_fun_tool
k_f = maya_fun_tool.open_file("E:/tmp/test2/DBXY_ep165_sc004_an.ma")
# k_f.get_cloth_sim("V:/03_Workflow/Assets/CFX/cloth")()
sim = k_f.get_cloth_sim("V:/03_Workflow/Assets/CFX/cloth")
sim()
# sim.sim_and_export()
