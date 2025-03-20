from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType

class FileStatus(BaseMixin):
    """
    Describe the state of a given file.
    """
    __tablename__ = "file_status"

    uuid : orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), unique=True, nullable=False)
    color = orm.mapped_column(sqlalchemy.String(7), nullable=False)
