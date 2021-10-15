import pymel.core
import pymel.core.language
env = pymel.core.language.Env()

plug_path = "C:/Users/TD/Source/Doodle/build/Ninja_release_plug/bin"
if env.envVars["path"].find(plug_path) < 0:
    env.envVars["path"] += ";"
    env.envVars["path"] += plug_path
print("\n".join(env.envVars["path"].split(";")))

pymel.core.system.loadPlugin(plug_path + "/doodle_plug.mll")
