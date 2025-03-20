from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType
from zou.app.models.preview_background_file import PreviewBackgroundFile as ZouPreviewBackgroundFile


class PreviewBackgroundFile(BaseMixin):
    """
    Describe a preview background file.
    """
    __tablename__ = "preview_background_file"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    is_default = orm.mapped_column(sqlalchemy.Boolean(), default=False, index=True)
    original_name = orm.mapped_column(sqlalchemy.String(250))
    extension = orm.mapped_column(sqlalchemy.String(6))
    file_size = orm.mapped_column(sqlalchemy.BigInteger(), default=0)

    def from_zou(self, preview_background_file: ZouPreviewBackgroundFile):
        self.uuid = preview_background_file.id
        self.name = preview_background_file.name
        self.archived = preview_background_file.archived
        self.is_default = preview_background_file.is_default
        self.original_name = preview_background_file.original_name
        self.extension = preview_background_file.extension
        self.file_size = preview_background_file.file_size
