import uuid
from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy import orm
import sqlalchemy
import json
from doodle.base import BaseMixin

from zou.app.models.entity import Entity as ZouEntity
from zou.app.models.entity import AssetInstanceLink as ZouAssetInstanceLink
from zou.app.models.entity import EntityLink as ZouEntityLink
from zou.app.models.entity import EntityConceptLink as ZouEntityConceptLink
from zou.app.models.entity import EntityVersion as ZouEntityVersion

ENTITY_STATUSES = [
    ("standby", "Stand By"),
    ("running", "Running"),
    ("complete", "Complete"),
    ("canceled", "Canceled"),
]


class AssetInstanceLink(BaseMixin):
    __tablename__ = "asset_instance_link"
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid")
    )
    asset_instance_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("asset_instance.uuid"),
    )

    def from_zou(self, asset_instance_link: ZouAssetInstanceLink):
        self.entity_id = asset_instance_link.entity_id
        self.asset_instance_id = asset_instance_link.asset_instance_id

        return self


class EntityLink(BaseMixin):
    __tablename__ = "entity_link"
    entity_in_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
        index=True,
    )
    entity_out_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
        index=True,
    )
    data = orm.mapped_column(sqlalchemy.TEXT())
    nb_occurences = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=1)
    label = orm.mapped_column(sqlalchemy.String(80), default="")

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "entity_in_id",
            "entity_out_id",
            name="entity_link_uc",
        ),
    )

    def from_zou(self, entity_link: ZouEntityLink):
        self.entity_in_id = entity_link.entity_in_id
        self.entity_out_id = entity_link.entity_out_id
        self.data = json.dumps(entity_link.data, ensure_ascii=False)
        self.nb_occurences = entity_link.nb_occurences
        self.label = entity_link.label

        return self


class EntityConceptLink(BaseMixin):
    __tablename__ = "entity_concept_link"
    entity_in_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
    )
    entity_out_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
    )

    def from_zou(self, entity_concept_link: ZouEntityConceptLink):
        self.entity_in_id = entity_concept_link.entity_in_id
        self.entity_out_id = entity_concept_link.entity_out_id

        return self


class EntityAssetExtend(BaseMixin):
    __tablename__ = "entity_asset_extend"
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid")
    )
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    ji_shu_lie = orm.mapped_column(sqlalchemy.Integer)
    deng_ji = orm.mapped_column(sqlalchemy.Text)
    gui_dang = orm.mapped_column(sqlalchemy.Integer)
    bian_hao = orm.mapped_column(sqlalchemy.Text)
    pin_yin_ming_cheng = orm.mapped_column(sqlalchemy.Text)
    ban_ben = orm.mapped_column(sqlalchemy.Text)
    ji_du = orm.mapped_column(sqlalchemy.Integer)
    kai_shi_ji_shu = orm.mapped_column(sqlalchemy.Integer) 

    def from_zou(self, entity: ZouEntity):
        self.entity_id = entity.id
        self.uuid = uuid.uuid4()
        l_json: dict = entity.data
        self.ji_shu_lie = l_json["ji_shu_lie"] if "ji_shu_lie" in l_json else None
        self.deng_ji = l_json["deng_ji"] if "deng_ji" in l_json else None
        self.gui_dang = l_json["gui_dang"] if "gui_dang" in l_json else None
        self.bian_hao = l_json["bian_hao"] if "bian_hao" in l_json else None
        self.pin_yin_ming_cheng = l_json["pin_yin_ming_cheng"] if "pin_yin_ming_cheng" in l_json else None
        self.ban_ben = l_json["ban_ben"] if "ban_ben" in l_json else None
        self.ji_du = l_json["ji_du"] if "ji_du" in l_json else None
        self.kai_shi_ji_shu = l_json["kai_shi_ji_shu"] if "kai_shi_ji_shu" in l_json else None

        return self

    @classmethod
    def has_extend(cls, entity: ZouEntity) -> bool:
        l_json: dict = entity.data
        return (
                "ji_shu_lie" in l_json
                or "deng_ji" in l_json
                or "gui_dang" in l_json
                or "bian_hao" in l_json
                or "pin_yin_ming_cheng" in l_json
                or "ban_ben" in l_json
                or "ji_du" in l_json
                or "kai_shi_ji_shu" in l_json
        )


class Entity(BaseMixin):
    """
    Base model to represent assets, shots, sequences, episodes and scenes.
    They have different meaning but they share the same behaviour toward
    tasks and files.
    """
    __tablename__ = "entity"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(160))
    code = orm.mapped_column(sqlalchemy.String(160))  # To store sanitized version of name
    description = orm.mapped_column(sqlalchemy.Text())
    shotgun_id = orm.mapped_column(sqlalchemy.Integer, nullable=False)
    canceled = orm.mapped_column(sqlalchemy.Boolean, default=False, nullable=False)

    nb_frames = orm.mapped_column(sqlalchemy.Integer)  # Specific to shots
    nb_entities_out = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    is_casting_standby = orm.mapped_column(sqlalchemy.Boolean, default=False, nullable=False)

    is_shared = orm.mapped_column(sqlalchemy.Boolean, default=False, nullable=False)

    status = orm.mapped_column(
        ChoiceType(ENTITY_STATUSES), default="running", nullable=False
    )

    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),
        nullable=False,
        index=True,
    )
    entity_type_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("asset_type.uuid"),
        nullable=False,
        index=True,
    )

    parent_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )  # sequence or episode

    source_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
        index=True,
        nullable=True,
    )  # if the entity is generated from another one (like shots from scene).

    preview_file_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("preview_file.uuid", name="fk_main_preview"),
    )

    ready_for = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task_type.uuid", name="fk_ready_for"),
    )

    created_by = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=True,
    )

    entities_out = orm.relationship(
        "Entity",
        secondary="entity_link",
        primaryjoin=(uuid == EntityLink.entity_in_id),
        secondaryjoin=(uuid == EntityLink.entity_out_id),
        backref="entities_in",
    )

    entity_concept_links = orm.relationship(
        "Entity",
        secondary="entity_concept_link",
        primaryjoin=(uuid == EntityConceptLink.entity_in_id),
        secondaryjoin=(uuid == EntityConceptLink.entity_out_id),
        lazy="joined",
    )

    instance_casting = orm.relationship(
        "AssetInstance", secondary="asset_instance_link", backref="shots"
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "name",
            "project_id",
            "entity_type_id",
            "parent_id",
            name="entity_uc",
        ),
    )

    def from_zou(self, entity: ZouEntity):
        self.uuid = entity.id
        self.name = entity.name
        self.code = entity.code
        self.description = entity.description
        self.shotgun_id = 0
        self.canceled = entity.canceled
        self.nb_frames = entity.nb_frames
        self.nb_entities_out = entity.nb_entities_out
        self.is_casting_standby = entity.is_casting_standby
        self.is_shared = entity.is_shared
        self.status = entity.status
        self.project_id = entity.project_id
        self.entity_type_id = entity.entity_type_id
        self.parent_id = entity.parent_id
        self.source_id = entity.source_id
        self.preview_file_id = entity.preview_file_id
        self.ready_for = entity.ready_for
        self.created_by = entity.created_by
        return self


class EntityVersion(BaseMixin):
    __tablename__ = "entity_version"
    name = orm.mapped_column(sqlalchemy.String(160), nullable=False)
    data = orm.mapped_column(sqlalchemy.TEXT())
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("person.uuid"), index=True
    )

    def from_zou(self, entity_version: ZouEntityVersion):
        self.name = entity_version.name
        self.data = json.dumps(entity_version.data, ensure_ascii=False)
        self.entity_id = entity_version.entity_id
        self.person_id = entity_version.person_id

        return self
