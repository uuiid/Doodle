import unreal


class doodle_lve:
    def __init__(self, ass_lev_name, ass_lev_path, ass_name, ass_path):
        # super().__init__()
        self.ass_lev_name = ass_lev_name
        self.ass_lev_path = ass_lev_path
        self.ass_name = ass_name
        self.ass_path = ass_path
        self.fps = 25
        self.start = 950
        self.end = 1200
        self.ass = None
        self.ass_lev = None

    def make_range(self, section):
        section.set_start_frame_bounded(True)
        section.set_end_frame_bounded(True)
        section.set_start_frame(self.start)
        section.set_end_frame(self.end)

    def create_level(self):
        self.ass_lev = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name=self.ass_lev_name,
            package_path=self.ass_lev_path,
            asset_class=unreal.World,
            factory=unreal.WorldFactory())

        self.ass = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name=self.ass_name,
            package_path=self.ass_path,
            asset_class=unreal.LevelSequence,
            factory=unreal.LevelSequenceFactoryNew())
        self.ass.set_display_rate(unreal.FrameRate(self.fps, 1))

        # 设置关卡序列的开始帧
        self.ass.make_range(self.start, self.end)
        self.ass.set_playback_start(self.start)
        self.ass.set_playback_end(self.end)

        self.ass.set_work_range_start(self.start / self.fps)
        self.ass.set_work_range_end(self.end / self.fps)

        self.ass.set_view_range_start(self.start / self.fps)
        self.ass.set_view_range_end(self.end / self.fps)
        # 添加摄像机关卡
        unreal.EditorLevelLibrary().load_level(
            "{path}{name}.{name}".format(path=self.ass_lev_path, name=self.ass_lev_name)
        )  # 打开创建的关卡

    def create_camera(self):
        # 添加相机剪辑轨道
        camera_cut_track = self.ass.add_master_track(unreal.MovieSceneCameraCutTrack)
        # 添加相机剪裁的可视
        camera_cut_section = camera_cut_track.add_section()
        self.make_range(camera_cut_section)

        camera_actor = unreal.EditorLevelLibrary().spawn_actor_from_class(
            unreal.CineCameraActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        camera_actor.set_actor_label("doodle_camera")
        camera_component = camera_actor.get_cine_camera_component()
        # ratio = math.tan(film_fov / 360.0 * math.pi) * 2

        # 添加一些相机熟悉
        filmback = camera_component.get_editor_property("filmback")
        # focal_length = camera_component.get_editor_property("current_focal_length")
        filmback.set_editor_property("sensor_height", 20.25)
        filmback.set_editor_property("sensor_width", 36.0)
        focus_settings = camera_component.get_editor_property("focus_settings")
        focus_settings.set_editor_property(
            "focus_method", unreal.CameraFocusMethod.DISABLE)

        # 添加相机绑定
        camera_binding = self.ass.add_possessable(camera_actor)
        camera_binding_id = self.ass.make_binding_id(
            camera_binding, unreal.MovieSceneObjectBindingSpace.LOCAL)
        camera_cut_section.set_camera_binding_id(camera_binding_id)

        # 添加transform_track轨道
        transform_track = camera_binding.add_track(unreal.MovieScene3DTransformTrack)
        transform_section = transform_track.add_section()
        self.make_range(transform_section)
        # 添加子组件绑定
        camera_component_binding = self.ass.add_possessable(camera_component)
        # 添加当前焦距
        camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
        camera_component_focal_track.set_property_name_and_path("当前焦距", "CurrentFocalLength")
        camera_component_focal_track_section = camera_component_focal_track.add_section()
        self.make_range(camera_component_focal_track_section)
        # channels = camera_component_focal_track_section.get_channels()[0]
        # channels.add_key(unreal.FrameNumber(950), 35, 0.0, unreal.SequenceTimeUnit.DISPLAY_RATE)
        # 添加当前光圈
        camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
        camera_component_focal_track.set_property_name_and_path("当前光圈", "CurrentAperture")
        camera_component_focal_track_section = camera_component_focal_track.add_section()
        self.make_range(camera_component_focal_track_section)

        # 添加手动聚焦
        camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
        camera_component_focal_track.set_property_name_and_path("手动聚焦距离(聚焦设置)",
                                                                "FocusSettings.ManualFocusDistance")
        camera_component_focal_track_section = camera_component_focal_track.add_section()
        self.make_range(camera_component_focal_track_section)

    # def create_camera_component_binding(self):
    #     pass

    def create_level_visibility(self):
        leve_track = self.ass.add_master_track(unreal.MovieSceneLevelVisibilityTrack)
        leve_visibility = leve_track.add_section()
        leve_visibility.set_visibility(unreal.LevelVisibility.VISIBLE)
        self.make_range(leve_visibility)

    def save(self):
        unreal.EditorLoadingAndSavingUtils.save_map(self.ass_lev, self.ass_lev_path + self.ass_lev_name)
        unreal.EditorAssetLibrary.save_directory(self.ass_path)

    def __call__(self):
        self.create_level()
        self.create_camera()
        self.create_level_visibility()
        self.save()
