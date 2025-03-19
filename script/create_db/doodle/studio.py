from doodle.base import BaseMixin
from sqlalchemy import orm
import sqlalchemy
import zou.app.models.studio as ZouStudio
from sqlalchemy_utils import UUIDType


class Studio(BaseMixin):
    """
    Describe a studio.
    """
    __tablename__ = "studio"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), unique=True, nullable=False)
    color = orm.mapped_column(sqlalchemy.String(7), nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False)

    def from_zou(self, studio: ZouStudio.Studio):
        self.name = studio.name
        self.color = studio.color
        self.archived = studio.archived
        self.uuid_id = studio.id
        return self
