from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.models.asset_instance import AssetInstance as ZouAssetInstance


class AssetInstance(BaseMixin):
    """
    An asset instance is the representation of an asset in a given shot or
    layout scene. It is useful for complex scenes where an asset needs extra
    treatments only related to the given shot or layout scene.
    An asset can have multiple instances in a scene or in a shot (ex: a sword in
    a battle field).
    """
    __tablename__ = "asset_instance"
    asset_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.id"),
        nullable=False,
        index=True,
    )
    name = orm.mapped_column(sqlalchemy.String(80))
    number = orm.mapped_column(sqlalchemy.Integer())
    description = orm.mapped_column(sqlalchemy.String(200))
    active = orm.mapped_column(sqlalchemy.Boolean(), default=True)
    data = orm.mapped_column(JSONB)

    scene_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("entity.id"), index=True
    )
    target_asset_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("entity.id"), index=True
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "asset_id",
            "target_asset_id",
            "scene_id",
            "number",
            name="asset_instance_uc",
        ),
    )

    # Do not use these column. They are deprecated and will be dropped in
    # upcoming version
    entity_id = orm.mapped_column(UUIDType(binary=False), sqlalchemy.ForeignKey("entity.id"))
    entity_type_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("entity_type.id")
    )

    def from_zou(self, zou_asset_instance: ZouAssetInstance):
        self.id = zou_asset_instance.id
        self.asset_id = zou_asset_instance.asset_id
        self.name = zou_asset_instance.name
        self.number = zou_asset_instance.number
        self.description = zou_asset_instance.description
        self.active = zou_asset_instance.active
        self.data = zou_asset_instance.data
        self.scene_id = zou_asset_instance.scene_id
        self.target_asset_id = zou_asset_instance.target_asset_id
        self.entity_id = zou_asset_instance.entity_id
        self.entity_type_id = zou_asset_instance.entity_type_id
        return self
