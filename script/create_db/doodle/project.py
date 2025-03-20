from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin

from doodle.person import ROLE_TYPES
from zou.app.models.project import Project as ZouProject
from zou.app.models.project import ProjectPersonLink as ZouProjectPersonLink
from zou.app.models.project import ProjectTaskTypeLink as ZouProjectTaskTypeLink
from zou.app.models.project import ProjectTaskStatusLink as ZouProjectTaskStatusLink
from zou.app.models.project import ProjectAssetTypeLink as ZouProjectAssetTypeLink
from zou.app.models.project import ProjectStatusAutomationLink as ZouProjectStatusAutomationLink
from zou.app.models.project import ProjectPreviewBackgroundFileLink as ZouProjectPreviewBackgroundFileLink

PROJECT_STYLES = [
    ("2d", "2D Animation"),
    ("2dpaper", "2D Animation (Paper)"),
    ("3d", "3D Animation"),
    ("2d3d", "2D/3D Animation"),
    ("ar", "Augmented Reality"),
    ("vfx", "VFX"),
    ("stop-motion", "Stop Motion"),
    ("motion-design", "Motion Design"),
    ("archviz", "Archviz"),
    ("commercial", "Commercial"),
    ("catalog", "Catalog"),
    ("immersive", "Immersive Experience"),
    ("nft", "NFT Collection"),
    ("video-game", "Video Game"),
    ("vr", "Virtual Reality"),
]


class ProjectPersonLink(BaseMixin):
    __tablename__ = "project_person_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),

        index=True,
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),

        index=True,
    )
    shotgun_id = orm.mapped_column(sqlalchemy.Integer)

    def from_zou(self, project_person_link: ZouProjectPersonLink):
        self.project_id = project_person_link.project_id
        self.person_id = project_person_link.person_id
        self.shotgun_id = project_person_link.shotgun_id
        return self


class ProjectTaskTypeLink(BaseMixin):
    __tablename__ = "project_task_type_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),

        index=True,
    )
    task_type_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task_type.uuid"),

        index=True,
    )
    priority = orm.mapped_column(sqlalchemy.Integer, default=None)

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "project_id", "task_type_id", name="project_tasktype_uc"
        ),
    )

    def from_zou(self, project_task_type_link: ZouProjectTaskTypeLink):
        self.project_id = project_task_type_link.project_id
        self.task_type_id = project_task_type_link.task_type_id
        self.priority = project_task_type_link.priority
        return self


class ProjectTaskStatusLink(BaseMixin):
    __tablename__ = "project_task_status_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),

        index=True,
    )
    task_status_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task_status.uuid"),

        index=True,
    )
    priority = orm.mapped_column(sqlalchemy.Integer, default=None)
    roles_for_board = orm.mapped_column(
        sqlalchemy.TEXT(),
        default=["user", "admin", "supervisor", "manager", "vendor"],
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "project_id", "task_status_id", name="project_taskstatus_uc"
        ),
    )

    def from_zou(self, project_task_status_link: ZouProjectTaskStatusLink):
        self.project_id = project_task_status_link.project_id
        self.task_status_id = project_task_status_link.task_status_id
        self.priority = project_task_status_link.priority
        self.roles_for_board = "[{}]".format(
            ", ".join([f'{role.code}' for role in project_task_status_link.roles_for_board]))
        return self


class ProjectAssetTypeLink(BaseMixin):
    __tablename__ = "project_asset_type_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("project.uuid")
    )
    asset_type_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("asset_type.uuid"),

    )

    def from_zou(self, project_asset_type_link: ZouProjectAssetTypeLink):
        self.project_id = project_asset_type_link.project_id
        self.asset_type_id = project_asset_type_link.asset_type_id
        return self


class ProjectStatusAutomationLink(BaseMixin):
    __tablename__ = "project_status_automation_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),

        index=True,
    )
    status_automation_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("status_automation.uuid"),

        index=True,
    )

    def from_zou(self, project_status_automation_link: ZouProjectStatusAutomationLink):
        self.project_id = project_status_automation_link.project_id
        self.status_automation_id = project_status_automation_link.status_automation_id
        return self


class ProjectPreviewBackgroundFileLink(BaseMixin):
    __tablename__ = "project_preview_background_file_link"
    project_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("project.uuid")
    )
    preview_background_file_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("preview_background_file.uuid"),

    )

    def from_zou(self, project_preview_background_file_link: ZouProjectPreviewBackgroundFileLink):
        self.project_id = project_preview_background_file_link.project_id
        self.preview_background_file_id = project_preview_background_file_link.preview_background_file_id
        return self


class Project(BaseMixin):
    """
    Describes a production the studio works on.
    """

    __tablename__ = "project"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), nullable=False, unique=True, index=True)
    code = orm.mapped_column(sqlalchemy.String(80))
    description = orm.mapped_column(sqlalchemy.Text())
    shotgun_id = orm.mapped_column(sqlalchemy.Integer)
    file_tree = orm.mapped_column(sqlalchemy.TEXT())
    data = orm.mapped_column(sqlalchemy.TEXT())
    has_avatar = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    fps = orm.mapped_column(sqlalchemy.String(10), default=25)
    ratio = orm.mapped_column(sqlalchemy.String(10), default="16:9")
    resolution = orm.mapped_column(sqlalchemy.String(12), default="1920x1080")
    production_type = orm.mapped_column(sqlalchemy.String(20), default="short")
    production_style = orm.mapped_column(
        ChoiceType(PROJECT_STYLES), default="2d3d", nullable=False
    )
    start_date = orm.mapped_column(sqlalchemy.Date())
    end_date = orm.mapped_column(sqlalchemy.Date())
    man_days = orm.mapped_column(sqlalchemy.Integer)
    nb_episodes = orm.mapped_column(sqlalchemy.Integer, default=0, nullable=False)
    episode_span = orm.mapped_column(sqlalchemy.Integer, default=0, nullable=False)
    max_retakes = orm.mapped_column(sqlalchemy.Integer, default=0, nullable=False)
    is_clients_isolated = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    is_preview_download_allowed = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    is_set_preview_automated = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    homepage = orm.mapped_column(sqlalchemy.String(80), default="assets")
    is_publish_default_for_artists = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    hd_bitrate_compression = orm.mapped_column(sqlalchemy.Integer, default=28)
    ld_bitrate_compression = orm.mapped_column(sqlalchemy.Integer, default=6)

    project_status_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("project_status.id"), index=True
    )

    default_preview_background_file_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("preview_background_file.id"),
        default=None,
        index=True,
    )

    team = orm.relationship("Person", secondary="project_person_link")
    asset_types = orm.relationship(
        "EntityType", secondary="project_asset_type_link"
    )
    task_statuses = orm.relationship(
        "TaskStatus", secondary="project_task_status_link"
    )
    task_types = orm.relationship(
        "TaskType", secondary="project_task_type_link"
    )
    status_automations = orm.relationship(
        "StatusAutomation", secondary="project_status_automation_link"
    )
    preview_background_files = orm.relationship(
        "PreviewBackgroundFile",
        secondary="project_preview_background_file_link",
    )

    def from_zou(self, project: ZouProject):
        self.uuid = project.id
        self.name = project.name
        self.code = project.code
        self.description = project.description
        self.shotgun_id = project.shotgun_id
        self.file_tree = project.file_tree
        self.data = project.data
        self.has_avatar = project.has_avatar
        self.fps = project.fps
        self.ratio = project.ratio
        self.resolution = project.resolution
        self.production_type = project.production_type
        self.production_style = project.production_style
        self.start_date = project.start_date
        self.end_date = project.end_date
        self.man_days = project.man_days
        self.nb_episodes = project.nb_episodes
        self.episode_span = project.episode_span
        self.max_retakes = project.max_retakes
        self.is_clients_isolated = project.is_clients_isolated
        self.is_preview_download_allowed = project.is_preview_download_allowed
        self.is_set_preview_automated = project.is_set_preview_automated
        self.homepage = project.homepage
        self.is_publish_default_for_artists = project.is_publish_default_for_artists
        self.hd_bitrate_compression = project.hd_bitrate_compression
        self.ld_bitrate_compression = project.ld_bitrate_compression
        self.project_status_id = project.project_status_id
        self.default_preview_background_file_id = project.default_preview_background_file_id
        return self
