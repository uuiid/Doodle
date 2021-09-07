# -*- coding: utf-8 -*-

import maya_fun_tool
k_f =  maya_fun_tool.open_file("F:/data/test/DBXY_163_057_1_sim_colth.ma")
# k_f.get_cloth_sim("V:/03_Workflow/Assets/CFX/cloth")()
sim = k_f.get_cloth_sim("V:/03_Workflow/Assets/CFX/cloth")
sim.sim_and_export()
