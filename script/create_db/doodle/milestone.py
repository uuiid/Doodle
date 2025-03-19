from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.utils import fields


class Milestone(BaseMixin):
    """
    Allow to set a milestone date to the production schedule.
    """

    date = db.Column(db.Date())
    name = db.Column(db.String(40), nullable=False)

    project_id = db.Column(
        UUIDType(binary=False), db.ForeignKey("project.id"), index=True
    )
    task_type_id = db.Column(
        UUIDType(binary=False), db.ForeignKey("task_type.id"), index=True
    )

    def present(self):
        return fields.serialize_dict(
            {
                "id": self.id,
                "date": self.date,
                "name": self.name,
                "project_id": self.project_id,
                "task_type_id": self.task_type_id,
            }
        )
