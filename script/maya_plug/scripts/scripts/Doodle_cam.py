import maya.mel as mel
import pymel.core as pm


def camBakeAim():
    cam = pm.ls(sl=True)[0]
    # print cam.nodeName()
    # print type(cam)
    if (cam):
        # focalLen = cam.getFocalLength()
        try:
            camloa = pm.spaceLocator()
            # print camloa
            # print type(camloa)
            pm.parent(camloa, cam)

            camloa.setTranslation([0, 0, 0])
            camloa.setRotation([0, 0, 0])

            newCam = pm.createNode('camera').getParent()
            newCam.setDisplayResolution(True)
            newCam.setDisplayGateMask(True)
            mel.eval('setAttr "{}.displayGateMaskOpacity" 1;'.format(cam.getShape().longName()))
            newCam.setOverscan(1)
            pm.rename(newCam, cam.nodeName())

            pointCon = pm.pointConstraint(camloa, newCam)
            orientCon = pm.orientConstraint(camloa, newCam)

            start = pm.playbackOptions(query=True, min=True)
            end = pm.playbackOptions(query=True, max=True)

            pm.copyKey(cam, attribute='focalLength', option='curve')
            try:
                pm.copyKey(cam, attribute='focalLength', option='curve')
            except:
                focalLen = cam.getFocalLength()
            else:
                try:
                    pm.pasteKey(newCam, attribute='focalLength')
                except:
                    focalLen = cam.getFocalLength()
                    newCam.setFocalLength(focalLen)
            pm.bakeResults(newCam, sm=True, t=(start, end))

            pm.delete(pointCon, orientCon, camloa)
        except:
            print("shi_Bai")
        else:
            print("cheng_Gong")
