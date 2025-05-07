from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
import datetime
from zou.app.models.comment import Comment as ZouComment
from zou.app.models.comment import CommentPreviewLink as ZouCommentPreviewLink
import zou.app.models.comment as ZouCommentModels
import json
class CommentPreviewLink(BaseMixin):
    __tablename__ = "comment_preview_link"
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid"),
        index=True,
    )
    preview_file_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("preview_file.uuid"),
        index=True,
    )
    def from_zou(self, ZouCommentPreviewLink: ZouCommentPreviewLink):
        self.comment_id = ZouCommentPreviewLink.comment
        self.preview_file_id = ZouCommentPreviewLink.preview_file
        return self


class CommentMentions(BaseMixin):
    __tablename__ = "comment_mentions"
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid"),
        index=True,
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        index=True,
    )
    def from_zou(self, ZouCommentMention):
        self.comment_id = ZouCommentMention[0]
        self.person_id = ZouCommentMention[1]
        return self


class CommentDepartmentMentions(BaseMixin):
    __tablename__ = "comment_department_mentions"
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid"),
        index=True,
    )
    department_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("department.uuid"),
        index=True,
    )
    def from_zou(self, ZouCommentDepartmentMention):
        self.comment_id = ZouCommentDepartmentMention[0]
        self.department_id = ZouCommentDepartmentMention[1]
        return self



class CommentAcknoledgments(BaseMixin):
    __tablename__ = "comment_acknoledgments"
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid"),
        index=True,
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        index=True,
    )
    def from_zou(self, ZouCommentAcknoledgment):
        self.comment_id = ZouCommentAcknoledgment[0]
        self.person_id = ZouCommentAcknoledgment[1]
        return self


class Comment(BaseMixin):
    """
    Comment can occur on any object but they are mainly used on tasks.
    In the case of comment tasks, they are linked to a task status and
    eventually to some preview files.
    The status means that comment leads to task status change. The preview file
    means that the comment relates to this preview in the context of the task.
    """

    __tablename__ = "comment"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )

    shotgun_id = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)

    object_id = orm.mapped_column(UUIDType(binary=True), nullable=False, index=True)
    object_type = orm.mapped_column(sqlalchemy.String(80), nullable=False, index=True)
    text = orm.mapped_column(sqlalchemy.Text())
    data = orm.mapped_column(sqlalchemy.TEXT())
    replies = orm.mapped_column(sqlalchemy.TEXT())
    checklist = orm.mapped_column(sqlalchemy.TEXT())
    pinned = orm.mapped_column(sqlalchemy.Boolean, nullable=False, default=False)
    links = orm.mapped_column(sqlalchemy.TEXT())

    created_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False, default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))
    updated_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False, default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))


    task_status_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task_status.uuid"), index=True
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False,
        index=True,
    )
    editor_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        default=None,
        index=True,
    )
    preview_file_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("preview_file.uuid")
    )
    previews = orm.relationship(
        "PreviewFile",
        secondary=CommentPreviewLink.__table__,
        backref="comments",
    )
    mentions = orm.relationship("Person", secondary=CommentMentions.__table__)
    department_mentions = orm.relationship(
        "Department", secondary=CommentDepartmentMentions.__table__
    )
    acknowledgements = orm.relationship(
        "Person", secondary=CommentAcknoledgments.__table__
    )
    attachment_files = orm.relationship("AttachmentFile", backref="comment")

    def from_zou(self, comment: ZouComment):
        self.uuid = comment.id
        self.shotgun_id = comment.shotgun_id
        self.object_id = comment.object_id
        self.object_type = comment.object_type
        self.text = comment.text
        self.data = json.dumps(comment.data, ensure_ascii=False)
        self.replies = json.dumps(comment.replies, ensure_ascii=False)
        self.checklist = json.dumps(comment.checklist, ensure_ascii=False)
        self.pinned = comment.pinned
        self.links = None if comment.links is None or len(comment.links) == 0 or comment.links[0] is None else json.dumps(comment.links, ensure_ascii=False)
        self.task_status_id = comment.task_status_id
        self.person_id = comment.person_id
        # self.editor_id = comment.editor_id
        self.preview_file_id = comment.preview_file_id
        
        self.created_at = comment.created_at
        self.updated_at = comment.updated_at
        return self