from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin
from zou.app.models.preview_file import PreviewFile as ZouPreviewFile
import json
import datetime

STATUSES = [
    ("processing", "Processing"),
    ("ready", "Ready"),
    ("broken", "Broken"),
]

VALIDATION_STATUSES = [
    ("validated", "Validated"),
    ("rejected", "Rejected"),
    ("neutral", "Neutral"),
]


class PreviewFile(BaseMixin):
    """
    Describes a file which is aimed at being reviewed. It is not a publication
    neither a working file.
    """
    __tablename__ = "preview_file"

    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(250))
    original_name = orm.mapped_column(sqlalchemy.String(250))
    revision = orm.mapped_column(sqlalchemy.Integer(), nullable=False, default=1)
    position = orm.mapped_column(sqlalchemy.Integer(), nullable=False, default=1)
    extension = orm.mapped_column(sqlalchemy.String(6))
    description = orm.mapped_column(sqlalchemy.Text())
    path = orm.mapped_column(sqlalchemy.String(400))
    source = orm.mapped_column(sqlalchemy.String(40))
    file_size = orm.mapped_column(sqlalchemy.BigInteger(), nullable=False, default=0)
    status = orm.mapped_column(
        ChoiceType(STATUSES), default="processing", nullable=False
    )
    validation_status = orm.mapped_column(
        ChoiceType(VALIDATION_STATUSES), default="neutral", nullable=False
    )
    annotations = orm.mapped_column(sqlalchemy.Text())
    width = orm.mapped_column(sqlalchemy.Integer(), nullable=False, default=0)
    height = orm.mapped_column(sqlalchemy.Integer(), nullable=False, default=0)
    duration = orm.mapped_column(sqlalchemy.Float, nullable=False, default=0)

    task_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task.uuid"), index=True
    )
    person_id = orm.mapped_column(UUIDType(binary=True), sqlalchemy.ForeignKey("person.uuid"))
    source_file_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("output_file.uuid")
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint("name", "task_id", "revision", name="preview_uc"),
    )

    shotgun_id = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)

    is_movie = orm.mapped_column(sqlalchemy.Boolean, nullable=False, default=False)  # deprecated
    url = orm.mapped_column(sqlalchemy.String(600))  # deprecated
    uploaded_movie_url = orm.mapped_column(sqlalchemy.String(600))  # deprecated
    uploaded_movie_name = orm.mapped_column(sqlalchemy.String(150))  # deprecated
    created_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False,
                                   default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))
    updated_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False,
                                   default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))

    def from_zou(self, preview_file: ZouPreviewFile):
        self.uuid = preview_file.id
        self.name = preview_file.name
        self.original_name = preview_file.original_name
        self.revision = preview_file.revision
        self.position = preview_file.position
        self.extension = preview_file.extension
        self.description = preview_file.description
        self.path = preview_file.path
        self.source = preview_file.source
        self.file_size = preview_file.file_size
        self.status = preview_file.status
        self.validation_status = preview_file.validation_status
        self.annotations = json.dumps(preview_file.annotations)
        self.width = preview_file.width
        self.height = preview_file.height
        self.duration = preview_file.duration
        self.task_id = preview_file.task_id
        self.person_id = preview_file.person_id
        self.source_file_id = preview_file.source_file_id
        self.shotgun_id = preview_file.shotgun_id
        self.is_movie = preview_file.is_movie
        self.url = preview_file.url
        self.uploaded_movie_url = preview_file.uploaded_movie_url
        self.uploaded_movie_name = preview_file.uploaded_movie_name

        self.created_at = preview_file.created_at
        self.updated_at = preview_file.updated_at

        return self
