# -*- coding: utf-8 -*-\n
import maya_fun_tool
k_f = maya_fun_tool.open_file()
k_f.config_ = """[{"export_path":"","only_sim":false,"path":"C:/Users/TD/Documents/maya/projects/default/scenes/ep001_sc002.ma","qcloth_assets_path":"V:/03_Workflow/Assets/CFX"}]"""
k_f.open()
import pymel.core
import pymel.all
pymel.all.mel.eval("""colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";""")

pymel.core.comm_play_blast_maya()
# pymel.core.playblast(
#     viewer=False,
#     startTime=1,
#     endTime=120,
#     filename="{path}/{base_name}_playblast_{start}-{end}"
#     .format(
#         path="D:/Doodle/cache/maya_play_blast/ep0001",
#         base_name="test",
#         start=1,
#         end=120
#     ),
#     percent=100,
#     quality=100,
#     widthHeight=(1920, 1280)
#     # editorPanelName="modelPanel4"
#     # offScreen=True
# )

# k_f()
# import pymel.core
# env = pymel.core.language.Env()
# print("\n".join(env.envVars["path"].split(";")))
# print("\npythonpath:\n")
# print("\n".join(env.envVars["pythonpath"].split(";")))

quit()
