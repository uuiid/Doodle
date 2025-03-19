from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.utils import fields, date_helpers
from zou.app.models.build_job import BuildJob as ZouBuildJob

STATUSES = [
    ("running", "Running"),
    ("failed", "Failed"),
    ("succeeded", "Succeeded"),
]

TYPES = [("archive", "Archive"), ("movie", "Movie")]


class BuildJob(BaseMixin):
    """
    A build job stores information about the state of the building
    of a given playlist.
    """

    status = orm.mapped_column(ChoiceType(STATUSES), default="running", nullable=False)
    job_type = orm.mapped_column(ChoiceType(TYPES), default="movie", nullable=False)
    ended_at = orm.mapped_column(sqlalchemy.DateTime)

    playlist_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("playlist.id"),
        nullable=False,
        index=True,
    )

    def from_zou(self, build_job: ZouBuildJob):
        self.uuid_id = build_job.id
        self.status = build_job.status
        self.job_type = build_job.job_type
        self.ended_at = build_job.ended_at
        self.playlist_id = build_job.playlist_id
        return self
