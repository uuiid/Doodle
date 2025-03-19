from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin

from zou.app.models.attachment_file import AttachmentFile as ZouAttachmentFile


class ChatMessage(BaseMixin):
    """
    Message shared in the entity chat feeds.
    """

    chat_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("chat.uuid_id"),
        nullable=False,
        index=True,
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid_id"),
        nullable=False,
        index=True,
    )
    text = orm.mapped_column(sqlalchemy.Text())
    attachment_files = sqlalchemy.relationship(
        "AttachmentFile", backref="chat_message", lazy="joined"
    )

    def from_zou(self, chat_message: ZouChatMessage):
        self.chat_id = chat_message.chat_id
        self.person_id = chat_message.person_id
        self.text = chat_message.text
        self.attachment_files = chat_message.attachment_files

        self.uuid_id = chat_message.id
