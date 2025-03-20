from sqlalchemy_utils import UUIDType, ChoiceType
from sqlalchemy.dialects.postgresql import JSONB
from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from doodle.department import Department
from zou.app.models.metadata_descriptor import MetadataDescriptor as ZouMetadataDescriptor
from zou.app.models.metadata_descriptor import DepartmentMetadataDescriptorLink as ZouDepartmentMetadataDescriptorLink


class DepartmentMetadataDescriptorLink(BaseMixin):
    __tablename__ = "department_metadata_descriptor_link"
    metadata_descriptor_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("metadata_descriptor.uuid"),
        index=True
    )
    department_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("department.uuid"),
        index=True
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "metadata_descriptor_id",
            "department_id",
            name="department_metadata_descriptor_link_uc",
        ),
    )

    def from_zou(self, department_metadata_descriptor_link: ZouDepartmentMetadataDescriptorLink):
        self.metadata_descriptor_id = department_metadata_descriptor_link.metadata_descriptor_id
        self.department_id = department_metadata_descriptor_link.department_id

        return self


METADATA_DESCRIPTOR_TYPES = [
    ("string", "String"),
    ("number", "Number"),
    ("list", "List"),
    ("taglist", "Taglist"),
    ("boolean", "Boolean"),
    ("checklist", "Checklist"),
]


class MetadataDescriptor(BaseMixin):
    """
    This models allow to identify which metadata are available for a given
    project and a given entity type.
    """

    __tablename__ = "metadata_descriptor"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    project_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("project.uuid"),
        nullable=False,
        index=True,
    )
    entity_type = orm.mapped_column(sqlalchemy.String(60), nullable=False, index=True)
    name = orm.mapped_column(sqlalchemy.String(120), nullable=False)
    data_type = orm.mapped_column(ChoiceType(METADATA_DESCRIPTOR_TYPES), nullable=False)
    field_name = orm.mapped_column(sqlalchemy.String(120), nullable=False)
    choices = orm.mapped_column(sqlalchemy.Text())
    for_client = orm.mapped_column(sqlalchemy.Boolean(), default=False, index=True)
    departments = orm.relationship(
        Department,
        secondary=DepartmentMetadataDescriptorLink.__table__,
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "project_id", "entity_type", "name", name="metadata_descriptor_uc"
        ),
        sqlalchemy.UniqueConstraint(
            "project_id",
            "entity_type",
            "field_name",
            name="metadata_descriptor_uc2",
        ),
    )

    def from_zou(self, metadata_descriptor: ZouMetadataDescriptor):
        self.project_id = metadata_descriptor.project_id
        self.entity_type = metadata_descriptor.entity_type
        self.name = metadata_descriptor.name
        self.data_type = metadata_descriptor.data_type
        self.field_name = metadata_descriptor.field_name
        self.choices = f"{metadata_descriptor.choices}"
        self.for_client = metadata_descriptor.for_client

        self.uuid = metadata_descriptor.id
        return self
