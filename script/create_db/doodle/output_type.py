from doodle.base import BaseMixin
from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType

class OutputType(BaseMixin):
    """
    Type of an output files (geometry, cache, etc.)
    """
    __tablename__ = "output_type"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), unique=True, nullable=False, index=True)
    short_name = orm.mapped_column(sqlalchemy.String(20), nullable=False)
