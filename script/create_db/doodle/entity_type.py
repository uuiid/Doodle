from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from doodle.task_type import TaskType
from zou.app.models.entity_type import EntityType as ZouEntityType
from zou.app.models.entity_type import TaskTypeAssetTypeLink as ZouTaskTypeAssetTypeLink


class TaskTypeAssetTypeLink(BaseMixin):
    __tablename__ = "task_type_asset_type_link"

    asset_type_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("asset_type.uuid"),
        index=True
    )
    task_type_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey(TaskType.uuid),
        index=True
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "asset_type_id",
            "task_type_id",
            name="task_type_asset_type_link_uc",
        ),
    )

    def from_zou(self, task_type_asset_type_link: ZouTaskTypeAssetTypeLink):
        self.asset_type_id = task_type_asset_type_link.asset_type_id
        self.task_type_id = task_type_asset_type_link.task_type_id

        return self


class EntityType(BaseMixin):
    """
    Type of entities. It can describe either an asset type, or tell if target
    entity is a shot, sequence, episode or layout scene.
    """
    __tablename__ = "asset_type"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(30), unique=True, nullable=False, index=True)
    short_name = orm.mapped_column(sqlalchemy.String(20))
    description = orm.mapped_column(sqlalchemy.Text())
    task_types = orm.relationship(
        TaskType,
        secondary=TaskTypeAssetTypeLink.__table__,
        lazy="joined",
    )
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False)

    def from_zou(self, entity_type: ZouEntityType):
        self.name = entity_type.name
        self.short_name = entity_type.short_name
        self.description = entity_type.description
        self.archived = entity_type.archived
        self.uuid = entity_type.id

        return self
