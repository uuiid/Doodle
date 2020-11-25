import unreal

# print("ok")


class ueScreenShot():
    frameRange = None
    frame = 1
    sequeue = None
    ass = ""

    def createScreenshot(self):
        # unreal.SoftObjectPath('/Game/NewLevelSequence.NewLevelSequence')
        # level = unreal.load_asset('/Game/NewLevelSequence.NewLevelSequence', unreal.LevelSequence)
        # sequeue = unreal.LevelSequencePlaybackController()
        # sequeue.set_active_level_sequence(level)
        if self.frame > self.frameRange[-1]:
            return None
        self.sequeue.jump_to_playback_position(unreal.FrameNumber(self.frame))
        unreal.AutomationLibrary.take_high_res_screenshot(3840, 2560, "test_shot_{:0>4d}.png".format(self.frame))
        self.frame = self.frame + 1

    def setRange(self):
        # unreal.SoftObjectPath('/Game/NewLevelSequence.NewLevelSequence')
        level = unreal.load_asset(self.ass, unreal.LevelSequence)
        self.sequeue = unreal.LevelSequencePlaybackController()
        self.sequeue.set_active_level_sequence(level)
        self.frameRange = range(level.get_playback_start(), level.get_playback_end(), 1)
        self.frame = self.frameRange[1]

    @staticmethod
    def getAss(_ass):
        ueScreenShot.ass = "'{}'".format(_ass.split("'")[-2])

ueScreenShotObj = ueScreenShot()
