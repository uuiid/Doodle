from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB
from sqlalchemy.orm import relationship

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


class Playlist(BaseMixin):
    """
    Describes a playlist. The goal is to review a set of shipped materials.
    """

    name = db.Column(db.String(80), nullable=False)
    shots = db.Column(JSONB)

    project_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("project.id"), index=True
    )
    episode_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("entity.id"), index=True
    )
    task_type_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("task_type.id"), index=True
    )
    for_client = db.Column(db.Boolean(), default=False, index=True)
    for_entity = db.Column(db.String(10), default="shot", index=True)
    is_for_all = db.Column(db.Boolean, default=False)

    build_jobs = relationship("BuildJob")

    __table_args__ = (
        db.UniqueConstraint(
            "name", "project_id", "episode_id", name="playlist_uc"
        ),
    )

    @classmethod
    def create_from_import(cls, data):
        del data["type"]
        del data["build_jobs"]
        previous_data = cls.get(data["id"])
        if previous_data is None:
            return (cls.create(**data), False)
        else:
            previous_data.update(data)
            return (previous_data, True)
