import pymel.core
import pymel.core.language
env = pymel.core.language.Env()

plug_path = "C:/Users/TD/Source/Doodle/build/Ninja_debug/bin"
if not env.envVars["path"].find(plug_path):
    env.envVars["path"] += plug_path

pymel.core.system.loadPlugin(plug_path + "/doodle_plug.mll")
