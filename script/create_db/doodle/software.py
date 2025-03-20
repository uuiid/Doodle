from doodle.base import BaseMixin

from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from sqlalchemy_utils import UUIDType


class Software(BaseMixin):
    """
    Describes software used by working files.
    """
    __tablename__ = "software"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), unique=True, nullable=False)
    short_name = orm.mapped_column(sqlalchemy.String(20), nullable=False)
    file_extension = orm.mapped_column(sqlalchemy.String(20), nullable=False)
    secondary_extensions = orm.mapped_column(sqlalchemy.TEXT())
