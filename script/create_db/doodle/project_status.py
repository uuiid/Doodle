from doodle.base import BaseMixin
from sqlalchemy import orm
import sqlalchemy
from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType
from zou.app.models.project_status import ProjectStatus as ZouProjectStatus


class ProjectStatus(BaseMixin):
    """
    Describes the state of the project (mainly open or closed).
    """
    __tablename__ = "project_status"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(20), unique=True, nullable=False, index=True)
    color = orm.mapped_column(sqlalchemy.String(7), nullable=False)

    def from_zou(self, project_status: ZouProjectStatus):
        self.name = project_status.name
        self.color = project_status.color
        self.uuid = project_status.id
        return self
