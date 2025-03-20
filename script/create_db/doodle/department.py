from doodle.base import BaseMixin
from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType
from zou.app.models.department import Department as ZouDepartment


class Department(BaseMixin):
    """
    Studio department like modeling, animation, etc.
    """
    __tablename__ = "department"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), unique=True, nullable=False)
    color = orm.mapped_column(sqlalchemy.String(7), nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)

    def from_zou(self, department: ZouDepartment):
        self.name = department.name
        self.color = department.color
        self.archived = department.archived
        self.uuid = department.id
        return self
