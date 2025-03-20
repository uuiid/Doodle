from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin

from zou.app.models.chat_message import ChatMessage as ZouChatMessage


class ChatMessage(BaseMixin):
    """
    Message shared in the entity chat feeds.
    """
    __tablename__ = "chat_message"

    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    chat_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("chat.uuid"),
        nullable=False,
        index=True,
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False,
        index=True,
    )
    text = orm.mapped_column(sqlalchemy.Text())
    attachment_files = orm.relationship(
        "AttachmentFile", backref="chat_message", lazy="joined"
    )

    def from_zou(self, chat_message: ZouChatMessage):
        self.chat_id = chat_message.chat_id
        self.person_id = chat_message.person_id
        self.text = chat_message.text
        self.attachment_files = chat_message.attachment_files

        self.uuid = chat_message.id
