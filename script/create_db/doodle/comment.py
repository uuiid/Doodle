from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


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

    shotgun_id = orm.mapped_column(sqlalchemy.Integer, nullable=False)

    object_id = orm.mapped_column(UUIDType(binary=True), nullable=False, index=True)
    object_type = orm.mapped_column(sqlalchemy.String(80), nullable=False, index=True)
    text = orm.mapped_column(sqlalchemy.Text())
    data = orm.mapped_column(sqlalchemy.TEXT())
    replies = orm.mapped_column(sqlalchemy.TEXT())
    checklist = orm.mapped_column(sqlalchemy.TEXT())
    pinned = orm.mapped_column(sqlalchemy.Boolean, nullable=False)
    links = orm.mapped_column(sqlalchemy.TEXT())

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
