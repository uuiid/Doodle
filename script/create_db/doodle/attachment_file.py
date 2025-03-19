from sqlalchemy_utils import UUIDType
from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.utils import fields
from zou.app.models.attachment_file import AttachmentFile as ZouAttachmentFile


class AttachmentFile(BaseMixin):
    """
    Describes a file which is attached to a comment.
    """
    __tablename__ = "attachment_file"

    uuid_id : orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(250))
    size = orm.mapped_column(sqlalchemy.Integer(), default=1)
    extension = orm.mapped_column(sqlalchemy.String(6))
    mimetype = orm.mapped_column(sqlalchemy.String(255))
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid_id"),
        index=True,
        nullable=True,
    )
    chat_message_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("chat_message.uuid_id"),
        index=True,
        nullable=True,
    )

    def from_zou(self, attachment_file: ZouAttachmentFile):
        self.name = attachment_file.name
        self.size = attachment_file.size
        self.extension = attachment_file.extension
        self.mimetype = attachment_file.mimetype
        self.comment_id = attachment_file.comment_id
        self.chat_message_id = attachment_file.chat_message_id

        self.uuid_id = attachment_file.id
        return self
