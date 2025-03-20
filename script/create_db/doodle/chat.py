from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.utils import fields
from zou.app.models.chat import ChatParticipant as ZouChatParticipant
from zou.app.models.chat import Chat as ZouChat


class ChatParticipant(BaseMixin):
    __tablename__ = "chat_participant"
    chat_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("chat.uuid")
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid")
    )

    def from_zou(self, chat_participant: ZouChatParticipant):
        self.chat_id = chat_participant.chat_id
        self.person_id = chat_participant.person_id
        return self


class Chat(BaseMixin):
    """
    Message shared in the entity chat feeds.
    """
    __tablename__ = "chat"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    object_id = orm.mapped_column(UUIDType(binary=True), nullable=False, index=True)
    object_type = orm.mapped_column(
        sqlalchemy.String(80), nullable=False, index=True, default="entity"
    )
    last_message = orm.mapped_column(sqlalchemy.DateTime, nullable=True)
    participants = orm.relationship(
        "Person", secondary="chat_participant", lazy="joined"
    )

    def from_zou(self, chat: ZouChat):
        self.uuid = chat.id
        self.object_id = chat.object_id
        self.object_type = chat.object_type
        self.last_message = chat.last_message
        self.participants = chat.participants
        return self
