import unreal

ass_lev_name = "test_lev"
ass_lev_path = "/Game/dev/"
ass_name = "test"
ass_path = "/Game/dev/"

# 添加基本关卡和关卡序列
ass_lev = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    asset_name=ass_lev_name,
    package_path=ass_lev_path,
    asset_class=unreal.World,
    factory=unreal.WorldFactory())


ass = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    asset_name=ass_name,
    package_path=ass_path,
    asset_class=unreal.LevelSequence,
    factory=unreal.LevelSequenceFactoryNew())
ass.set_display_rate(unreal.FrameRate(25, 1))

# 设置关卡序列的开始帧
ass.make_range(1001, 1200)
ass.set_playback_start(1001)
ass.set_playback_end(1200)

ass.set_work_range_start(40)
ass.set_work_range_end(30)

ass.set_view_range_start(30)
ass.set_view_range_end(30)

# 添加相机剪辑轨道
camera_cut_track = ass.add_master_track(unreal.MovieSceneCameraCutTrack)
# 添加相机剪裁的可视
camera_cut_section = camera_cut_track.add_section()
camera_cut_section.set_start_frame_bounded(True)
camera_cut_section.set_end_frame_bounded(True)
camera_cut_section.set_start_frame(1001)
camera_cut_section.set_end_frame(1200)

# 添加摄像机关卡
unreal.EditorLevelLibrary().load_level(
    "{path}{name}.{name}".format(path=ass_lev_path, name=ass_lev_name)
)  # 打开创建的关卡

camera_actor = unreal.EditorLevelLibrary().spawn_actor_from_class(
    unreal.CineCameraActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
camera_actor.set_actor_label("doodle_camera")
camera_component = camera_actor.get_cine_camera_component()
# ratio = math.tan(film_fov / 360.0 * math.pi) * 2

# 添加一些相机熟悉
filmback = camera_component.get_editor_property("filmback")
focal_length = camera_component.get_editor_property("current_focal_length")
filmback.set_editor_property("sensor_height", 20.25)
filmback.set_editor_property("sensor_width", 36.0)
focus_settings = camera_component.get_editor_property("focus_settings")
focus_settings.set_editor_property(
    "focus_method", unreal.CameraFocusMethod.DISABLE)

# 添加相机绑定
camera_binding = ass.add_possessable(camera_actor)
# 添加transform_track轨道
transform_track = camera_binding.add_track(unreal.MovieScene3DTransformTrack)
transform_section = transform_track.add_section()
transform_section.set_start_frame_bounded(True)
transform_section.set_end_frame_bounded(True)
transform_section.set_start_frame(1001)
transform_section.set_end_frame(1200)
# 添加子组件绑定
camera_component_binding = ass.add_possessable(camera_component)
# 添加当前焦距
camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
camera_component_focal_track.set_property_name_and_path("当前焦距", "CurrentFocalLength")
camera_component_focal_track_section = camera_component_focal_track.add_section()
camera_component_focal_track_section.set_start_frame_bounded(True)
camera_component_focal_track_section.set_end_frame_bounded(True)
camera_component_focal_track_section.set_start_frame(1001)
camera_component_focal_track_section.set_end_frame(1200)
# channels = camera_component_focal_track_section.get_channels()[0]
# channels.add_key(unreal.FrameNumber(950), 35, 0.0, unreal.SequenceTimeUnit.DISPLAY_RATE)
# 添加当前光圈
camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
camera_component_focal_track.set_property_name_and_path("当前光圈", "CurrentAperture")
camera_component_focal_track_section = camera_component_focal_track.add_section()
camera_component_focal_track_section.set_start_frame_bounded(True)
camera_component_focal_track_section.set_end_frame_bounded(True)
camera_component_focal_track_section.set_start_frame(1001)
camera_component_focal_track_section.set_end_frame(1200)

# 添加手动聚焦
camera_component_focal_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
camera_component_focal_track.set_property_name_and_path("手动聚焦距离(聚焦设置)", "FocusSettings.ManualFocusDistance")
camera_component_focal_track_section = camera_component_focal_track.add_section()
camera_component_focal_track_section.set_start_frame_bounded(True)
camera_component_focal_track_section.set_end_frame_bounded(True)
camera_component_focal_track_section.set_start_frame(1001)
camera_component_focal_track_section.set_end_frame(1200)
# ass.add_possessable(camera_component.get_editor_property("current_focal_length"))
# camera_component_track = camera_component_binding.add_track(unreal.MovieScenePropertyTrack)
# camera_component_track.add_section()



camera_binding_id = ass.make_binding_id(
    camera_binding, unreal.MovieSceneObjectBindingSpace.LOCAL)
camera_cut_section.set_camera_binding_id(camera_binding_id)
# 添加关卡可视性
# leveTrack = ass.add_master_track(unreal.MovieSceneLevelVisibilityTrack)
# leveVisibility = leveTrack.add_section()
# leveVisibility.set_visibility(unreal.LevelVisibility.VISIBLE)
# leveVisibility.set_start_frame_bounded(True)
# leveVisibility.set_end_frame_bounded(True)
# leveVisibility.set_start_frame(1001)
# leveVisibility.set_end_frame(1200)

# 添加子场景
subTrack = ass.add_master_track(unreal.MovieSceneSubTrack)
subScene = subTrack.add_section()
subScene.set_start_frame_bounded(True)
subScene.set_end_frame_bounded(True)
subScene.set_start_frame(1001)
subScene.set_end_frame(1200)
# 保存函数
unreal.EditorLoadingAndSavingUtils.save_map(ass_lev, ass_lev_path + ass_lev_name)
unreal.EditorAssetLibrary.save_directory(ass_path)